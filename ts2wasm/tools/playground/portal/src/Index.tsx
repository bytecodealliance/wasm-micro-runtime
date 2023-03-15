/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { useEffect, useState } from "react";
import * as dom from 'react-dom';
import { Divider, Typography, Button, Card, Col, Row, Space, Layout } from "antd";

const { Header, Footer, Sider, Content } = Layout;

const { Title } = Typography;

const Index: React.FC<{}> = () => {

  const options = {
    selectOnLineNumbers: true
  };

  return (
    <div className="index">

    </div>
  );
};
export default Index;
