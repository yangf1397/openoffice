/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/
#ifndef GCONXHP_HXX
#define GCONXHP_HXX
#include "gCon.hxx"



/*****************************************************************************
 *************************   G C O N X H P . H X X   *************************
 *****************************************************************************
 * This is the class definition header xhp converter
 *****************************************************************************/



/********************   C L A S S   D E F I N I T I O N   ********************/
class convert_xhp : public convert_gen_impl
{
  public:
    convert_xhp(l10nMem& crMemory);
    ~convert_xhp();

    void startCollectData(std::string sType, std::string& sCollectedText);
    void stopCollectData(std::string sType, std::string& sCollectedText);

  private:
	std::string msMergeType;
    std::string msTag;

    void execute();
};
#endif
