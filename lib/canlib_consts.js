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


function convertAcceptanceFilter(filter_obj) {
  let code;
  let mask;
  let ext = filter_obj.ext;

  if ((filter_obj.code) && (filter_obj.mask)) {
    // No action necessary, filter is already in format required by
    // CANlib API
    code = filter_obj.code;
    mask = filter_obj.mask;

  } else if ((filter_obj.fromID) && (filter_obj.toID)){
    // Convert to code and mask format
    code = filter_obj.fromID & filter_obj.toID;
    mask = ~(filter_obj.fromID ^ filter_obj.toID);
  } else if (filter_obj.id) {
    // Split start/stop range and convert them to acceptance code + mask
    let fromID = parseInt(filter_obj.id.split(' ')[0], 16);
    let toID = parseInt(filter_obj.id.split(' ')[1], 16);
    // Convert to code and mask format
    code = fromID & toID;
    mask = ~(fromID ^ toID);
  } else {
    throw new Error("Unknown or unspecified filter format.");
  }

  return {code, mask, ext};
}


module.exports = {
  toCanlibMsg: toCanlibMsg,
  convertAcceptanceFilter: convertAcceptanceFilter,
};
