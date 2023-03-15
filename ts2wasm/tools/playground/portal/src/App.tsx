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

const { Header, Footer, Sider, Content } = Layout;

function App() {
  const editorCountainerRef = useRef(null);
  const editorRef = useRef(null);
  const [result, setResult] = useState<any>();
  const [isModalOpen, setIsModalOpen] = useState(false);

  const resultCountainerRef = useRef(null);
  const resultRef = useRef(null);

  const [userName, setUserName] = useState("");
  const [userComments, setUserComments] = useState("");
  const [opt, setOpt] = useState(true);
  const [resFormat, setResFormat] = useState("S-expression");

  const serverUrl = "http://" + import.meta.env.VITE_SERVER_IP
    + ":" + import.meta.env.VITE_SERVER_PORT;

  const options = {
    selectOnLineNumbers: true
  };

  const showModal = () => {
    setIsModalOpen(true);
  };

  const handleOk = async () => {
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

  const handleCancel = () => {
    setIsModalOpen(false);
  };

  const userInputChange = (e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => {
    setUserName(e.target.value);
  }

  const commentInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    setUserComments(e.target.value);
  }

  const default_ts_code =
    `export function test(x: number, y: number) {
  return x + y;
}
`
  const doCompile = async () => {
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
      setResult(resData.error);
      message.error('Failed to build the source code');
      return;
    }

    message.success(`Successfully built the source code in ${resData.duration / 1000}s`);
    setResult(resData.content);
  }

  const toggleOpt = () => {
    setOpt(!opt);
  }

  const handleFormatChange = (value: string) => {
    setResFormat(value);
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
    if (result && resultRef.current) {
      (resultRef.current as any).setValue(result);
    }
  }, [result])

  return (
    <div className="container mx-auto h-full">
      <Modal title="Feedback" open={isModalOpen} onOk={handleOk} onCancel={handleCancel}>
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
      <div className="h-16 bg-sky-500 flex items-center">
        <div className="static flex items-center justify-between">
          <span className="grow font-mono ml-10 h-full text-2xl text-white inline-block align-middle">Ts2wasm Playground</span>
          <div className="absolute mr-10 right-0">
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
