'use strict';

const canMSG_MASK = 0x00FF;
const canMSG_RTR = 0x0001;
const canMSG_STD = 0x0002;
const canMSG_EXT = 0x0004;

function toCanlibMsg(msg) {
  msg.flag = msg.ext ? canMSG_EXT : canMSG_STD;
  msg.dlc = msg.buf.length;

  return msg;
}


module.exports = {
  toCanlibMsg: toCanlibMsg,
};
