/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import "./App.css";
import "antd/dist/reset.css";
import { Divider, Typography, Button, Card, Col, Row, Space, Layout, Modal, Input, message, Switch, Select } from "antd";
import { BrowserRouter as Router, Route } from "react-router-dom";
import * as monaco from 'monaco-editor';
import { useEffect, useRef, useState } from "react";
import TextArea from "antd/es/input/TextArea";
import { CheckOutlined, CloseOutlined, UserOutlined } from "@ant-design/icons";
// @ts-ignore
import { importObject, setWasmMemory } from './libdyntype';

const { Header, Footer, Sider, Content } = Layout;

function App() {
  const editorCountainerRef = useRef(null);
  const editorRef = useRef(null);
  const [wasmText, setWasmText] = useState<any>();
  const [wasmBuffer, setWasmBuffer] = useState<any>();
  const [isFeedbackModalOpen, setIsModalOpen] = useState(false);
  const [isRunModalOpen, setIsRunModalOpen] = useState(false);
  const [currWasmInst, setCurrWasmInst] = useState<WebAssembly.Instance | null>(null);
  const [selectFunc, setSelectFunc] = useState<string>("");
  const [params, setParams] = useState<string>("");

  const resultCountainerRef = useRef(null);
  const resultRef = useRef(null);

  const [userName, setUserName] = useState("");
  const [userComments, setUserComments] = useState("");
  const [opt, setOpt] = useState(true);
  const [resFormat, setResFormat] = useState("S-expression");
  const [sampleList, setSampleList] = useState([]);

  useEffect(() => {
    fetch(serverUrl + `/samples`)
      .then((response) => {
        return response.json()
      })
      .then((data: any) => {
        setSampleList(data);
      })
  }, [])

  const serverUrl = "http://" + import.meta.env.VITE_SERVER_IP
    + ":" + import.meta.env.VITE_SERVER_PORT;

  const options = {
    selectOnLineNumbers: true
  };

  const showModal = () => {
    setIsModalOpen(true);
  };

  const handleFeedbackOk = async () => {
    if (!userComments) {
      message.error('Please input some content');
      return;
    }

    let data = {
      user: userName ? userName : 'unknown',
      suggests: userComments
    };

    try {
      const response = await fetch(serverUrl + `/feedback`, {
        method: 'POST',
        mode: 'cors',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
      });
      if (response.ok) {
        setUserName("");
        setUserComments("");
        message.info('Thanks for the feedback!');
      }
      else {
        message.error('Server error');
      }
    }
    catch {
      message.error('Server error');
    }

    setIsModalOpen(false);
  };

  const handleFeedbackCancel = () => {
    setIsModalOpen(false);
  };

  const handleRunCancel = () => {
    setIsRunModalOpen(false);
  };

  const userInputChange = (e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => {
    setUserName(e.target.value);
  }

  const commentInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    setUserComments(e.target.value);
  }

  const handleFuncChange = (value: string) => {
    setSelectFunc(value);
  }

  const default_ts_code =
    `export function test(x: number, y: number) {
  return x + y;
}
`
  const doCompile = async () => {
    setCurrWasmInst(null);
    setSelectFunc("");
    setParams("");
    const payload = JSON.stringify({
      code: (editorRef.current as any).getValue(),
      options: {
        opt: opt,
        format: resFormat
      }
    })
    const response = await fetch(serverUrl + `/compile`, {
      method: 'POST',
      mode: 'cors',
      headers: {
        'Content-Type': 'application/json'
      },
      body: payload
    });
    const resData = await response.json();
    if (resData.error) {
      setWasmText(resData.error);
      message.error('Failed to build the source code');
      return;
    }

    message.success(`Successfully built the source code in ${resData.duration / 1000}s`);
    setWasmText(resData.content);

    const decodedWasmModule = atob(resData.wasm);
    const wasmBuffer = new Uint8Array([...decodedWasmModule].map((char => char.charCodeAt(0))));

    setWasmBuffer(wasmBuffer);
  }

  const doRun = () => {
    const wasmExports = currWasmInst!.exports;
    const func = wasmExports[selectFunc];
    if (!func) {
      message.error('Please select a function');
      return;
    }
    setWasmMemory(wasmExports.default);

    const paramsArr = params.split(',')
      .map((p) => p.trim())
      .filter((p) => p !== '');
    if (paramsArr.length !== (func as Function).length) {
      message.error('Parameter number mismatch');
      return;
    }

    const valid = paramsArr.every((p) => {
      return !isNaN(p as unknown as number)
    })
    if (!valid) {
      message.error('parameter format error');
      return;
    }

    try {
      const result = (func as any)(...paramsArr.map((p) => parseFloat(p)));
      message.success(`The result is ${result}`);
    }
    catch (e) {
      message.error(`Failed to run the function: ${e}`);
    }
  }

  const openRunWindow = async () => {
    try {
      const wasmModule = await WebAssembly.compile(wasmBuffer);
      const wasmInstance = await WebAssembly.instantiate(wasmModule, importObject);
      setCurrWasmInst(wasmInstance);
    }
    catch (e) {
      message.error(`Failed to instantiate wasm module: ${e}`);
      return;
    }

    setIsRunModalOpen(true);
  }

  const toggleOpt = () => {
    setOpt(!opt);
  }

  const handleFormatChange = (value: string) => {
    setResFormat(value);
  }

  const handleSampleChange = async (value: string) => {
    const resp = await fetch(serverUrl + `/samples/${value}`);
    const code = await resp.text();

    (editorRef.current as any).setValue(code);
  }

  const handleParamChange = (e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => {
    setParams(e.target.value);
  }

  const paramPlaceHolder = (wasmInst: WebAssembly.Instance) => {
    if (!wasmInst) {
      return "";
    }
    const wasmExports = wasmInst.exports;
    const func = wasmExports[selectFunc] as Function;
    if (!func) {
      return "please select a function";
    }

    let paramStr = `${func.length} args required`;

    if (func.length) {
      paramStr += `, please separate with ','`
    }

    return paramStr;
  }

  useEffect(() => {
    if (editorCountainerRef.current) {
      (editorRef.current as any) = monaco.editor.create(editorCountainerRef.current, {
        value: default_ts_code,
        language: 'typescript',
        minimap: {
          enabled: false
        }
      });
    }

    return () => {
      (editorRef.current as any).dispose();
    };
  }, [editorCountainerRef])

  useEffect(() => {
    if (resultCountainerRef.current) {
      (resultRef.current as any) = monaco.editor.create(resultCountainerRef.current, {
        value: '',
        language: 'webassembly',
        readOnly: true,
        minimap: {
          enabled: false
        }
      });
    }

    return () => {
      (resultRef.current as any).dispose();
    };
  }, [resultCountainerRef])

  useEffect(() => {
    if (wasmText && resultRef.current) {
      (resultRef.current as any).setValue(wasmText);
    }
  }, [wasmText])

  return (
    <div className="container mx-auto h-full">
      <Modal title="Feedback" open={isFeedbackModalOpen} onOk={handleFeedbackOk} onCancel={handleFeedbackCancel}>
        <span>Name:</span>
        <Input
          value={userName}
          prefix={<UserOutlined className="site-form-item-icon" />}
          onChange={userInputChange}
          allowClear
          placeholder="Optional" />
        <br />
        <span>Comments:</span>
        <TextArea
          value={userComments}
          allowClear
          onChange={commentInputChange}
          showCount
          rows={4} />
        <br />
      </Modal>
      <Modal title="Run Wasm Function" open={isRunModalOpen} onCancel={handleRunCancel}
        footer={[]}>
        <Row>
          <Col span={4}>
            <span className="grow font-mono mt-2">Function:</span>
          </Col>
          <Col span={20}>
            <Select className="ml-5"
              value={selectFunc}
              style={{ width: 300 }}
              onChange={handleFuncChange}
              options={
                currWasmInst && Object.keys(currWasmInst!.exports)
                  .filter((key) => {
                    /* filter out non-function objects (memory/global/table) */
                    return typeof currWasmInst!.exports[key] === 'function';
                  })
                  .map((f) => {
                    return { value: f, label: f };
                  }) || []
              }
            />
          </Col>
        </Row>
        <Row className="mt-2">
          <Col span={4}>
            <span className="grow font-mono">Parameter:</span>
          </Col>
          <Col span={20}>
            <Input
              value={params}
              onChange={handleParamChange}
              className="ml-5" style={{ width: 300 }}
              placeholder={paramPlaceHolder(currWasmInst!)}
            />
          </Col>
        </Row>
        <Button className="mt-2" type="primary" onClick={doRun} disabled={!(currWasmInst && selectFunc)}>Run</Button>
      </Modal>
      <div className="h-16 bg-sky-500 flex items-center">
        <div className="static flex items-center justify-between">
          <span className="grow font-mono ml-10 h-full text-2xl text-white inline-block align-middle">Ts2wasm Playground</span>
          <div className="absolute mr-10 right-0">
            <span className="grow font-mono ml-3 h-full text-2 text-white inline-block align-middle">Samples: </span>
            <Select
              className="ml-2"
              style={{ width: 200 }}
              onChange={handleSampleChange}
              options={
                sampleList?.map((sample) => {
                  return { value: sample, label: sample };
                }) || []
              }
            />
            <span className="grow font-mono ml-3 h-full text-2 text-white inline-block align-middle">Format: </span>
            <Select
              className="ml-2"
              defaultValue="S-expression"
              style={{ width: 130 }}
              onChange={handleFormatChange}
              options={[
                { value: 'S-expression', label: 'S-expression' },
                { value: 'Stack-IR', label: 'Stack-IR' },
              ]}
            />
            <span className="grow font-mono ml-3 h-full text-2 text-white inline-block align-middle">Opt: </span>
            <Switch
              className="ml-2"
              style={{ backgroundColor: opt ? 'seagreen' : 'gray' }}
              checkedChildren={<CheckOutlined />}
              unCheckedChildren={<CloseOutlined />}
              onChange={toggleOpt}
              defaultChecked
            />
            <Button className="ml-5" onClick={doCompile}>Compile</Button>
            <Button className="ml-5 disabled:bg-gray-400" onClick={openRunWindow} disabled={!wasmBuffer}>Run</Button>
            <Button className="ml-5 " onClick={showModal}>Feedback</Button>
          </div>
        </div>
      </div>
      <div className="h-5/6 mt-2 mb-2">
        <div className="flex h-full">
          <div className="w-1/2 ml-2 mr-1 h-full" ref={editorCountainerRef}></div>
          <div className="w-1/2 ml-1 mr-2 h-full" ref={resultCountainerRef}></div>
        </div>
      </div>
    </div>
  );
}

export default App;
