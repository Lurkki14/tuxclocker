#!/usr/bin/env sh
cppPat=(-name "*.cpp" -o -name "*.hpp" -o -name "*.h")
clang-format -i $(
	(find src/tuxclocker-qt/ ${cppPat[@]}
	find src/tuxclockerd/ ${cppPat[@]}
	find src/plugins/ ${cppPat[@]}
	find src/lib/ ${cppPat[@]}
	find src/include/ -maxdepth 1 ${cppPat[@]}))
