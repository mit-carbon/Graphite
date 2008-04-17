PIN_BIN=../Bin/pin
PIN_TOOL=test
TOOL_OPTS=""
TOOL_OUT=test.out

rm -f ${TOOL_OUT}
${PIN_BIN} -t ${PIN_TOOL} ${TOOL_OPTS} -- ls > ${TOOL_OUT}
cat ${TOOL_OUT} | less
