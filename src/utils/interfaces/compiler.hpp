// Copyright 2020 xensik. All rights reserved.
//
// Use of this source code is governed by a GNU GPLv3 license
// that can be found in the LICENSE file.

#ifndef _GSC_COMPILER_H_
#define _GSC_COMPILER_H_

namespace gsc
{

class compiler
{
public:
	virtual void compile(std::string& buffer) = 0;
	virtual auto output() -> std::vector<std::shared_ptr<function>> = 0;
};

} // namespace gsc

#endif // _GSC_COMPILER_H_
