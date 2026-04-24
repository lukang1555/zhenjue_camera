"use strict";

const {
  AI_TO_PLC_SWITCH_BITS,
  PLC_TO_AI_STATUS_BITS,
  AI_TO_PLC_ANALOG_FIELDS,
  PLC_TO_AI_TELEMETRY_300,
  PLC_TO_AI_TELEMETRY_400,
} = require("./constants");

function ensure8Bytes(bytes) {
  if (!Array.isArray(bytes) || bytes.length !== 8) {
    throw new Error("bytes must be an array of length 8");
  }
}

function clampByte(v) {
  var n = Number(v);
  if (Number.isNaN(n)) return 0;
  if (n < 0) return 0;
  if (n > 255) return 255;
  return n & 0xff;
}

function readInt16LE(low, high) {
  var value = (clampByte(low) | (clampByte(high) << 8)) & 0xffff;
  return value & 0x8000 ? value - 0x10000 : value;
}

function writeInt16LE(value) {
  var n = Number(value) || 0;
  if (n < -32768) n = -32768;
  if (n > 32767) n = 32767;
  var unsigned = n < 0 ? 0x10000 + n : n;
  return [unsigned & 0xff, (unsigned >> 8) & 0xff];
}

function decodeBitsByMap(byteValue, bitMap) {
  var out = {};
  Object.keys(bitMap).forEach(function (bitKey) {
    var bitIdx = Number(bitKey.replace("bit", ""));
    out[bitMap[bitKey]] = ((byteValue >> bitIdx) & 0x01) === 1;
  });
  return out;
}

function encodeBitsByMap(dataObj, bitMap) {
  var byteValue = 0;
  Object.keys(bitMap).forEach(function (bitKey) {
    var bitIdx = Number(bitKey.replace("bit", ""));
    var fieldName = bitMap[bitKey];
    if (dataObj && dataObj[fieldName]) {
      byteValue |= 1 << bitIdx;
    }
  });
  return byteValue & 0xff;
}

function decodeAiToPlcSwitch(bytes) {
  ensure8Bytes(bytes);
  var out = {};
  ["data1", "data2", "data3", "data4", "data5", "data6", "data7", "data8"].forEach(function (dataKey) {
    var idx = Number(dataKey.replace("data", "")) - 1;
    var bitMap = AI_TO_PLC_SWITCH_BITS[dataKey];
    var part = decodeBitsByMap(clampByte(bytes[idx]), bitMap);
    Object.assign(out, part);
  });
  return out;
}

function encodeAiToPlcSwitch(commandObj) {
  var bytes = [0, 0, 0, 0, 0, 0, 0, 0];
  ["data1", "data2", "data3", "data4", "data5", "data6", "data7", "data8"].forEach(function (dataKey) {
    var idx = Number(dataKey.replace("data", "")) - 1;
    bytes[idx] = encodeBitsByMap(commandObj || {}, AI_TO_PLC_SWITCH_BITS[dataKey]);
  });
  return bytes;
}

function decodePlcToAiStatus(bytes) {
  ensure8Bytes(bytes);
  return decodeBitsByMap(clampByte(bytes[0]), PLC_TO_AI_STATUS_BITS.data1);
}

function encodeAiToPlcAnalog(analogObj) {
  var bytes = [0, 0, 0, 0, 0, 0, 0, 0];
  Object.keys(AI_TO_PLC_ANALOG_FIELDS).forEach(function (dataKey) {
    var idx = Number(dataKey.replace("data", "")) - 1;
    var field = AI_TO_PLC_ANALOG_FIELDS[dataKey];
    bytes[idx] = clampByte(analogObj && analogObj[field]);
  });
  return bytes;
}

function decodeAiToPlcAnalog(bytes) {
  ensure8Bytes(bytes);
  var out = {};
  Object.keys(AI_TO_PLC_ANALOG_FIELDS).forEach(function (dataKey) {
    var idx = Number(dataKey.replace("data", "")) - 1;
    out[AI_TO_PLC_ANALOG_FIELDS[dataKey]] = clampByte(bytes[idx]);
  });
  return out;
}

function decodeTelemetryBySchema(bytes, schema) {
  ensure8Bytes(bytes);
  var out = {};
  schema.forEach(function (item) {
    var low = bytes[item.dataLow - 1];
    var high = bytes[item.dataHigh - 1];
    var raw = readInt16LE(low, high);
    out[item.keyRaw] = raw;
    out[item.keyValue] = Number((raw * item.scale).toFixed(3));
  });
  return out;
}

function encodeTelemetryBySchema(dataObj, schema) {
  var bytes = [0, 0, 0, 0, 0, 0, 0, 0];
  schema.forEach(function (item) {
    var raw = dataObj && typeof dataObj[item.keyRaw] === "number"
      ? dataObj[item.keyRaw]
      : Math.round((Number(dataObj && dataObj[item.keyValue]) || 0) / item.scale);
    var pair = writeInt16LE(raw);
    bytes[item.dataLow - 1] = pair[0];
    bytes[item.dataHigh - 1] = pair[1];
  });
  return bytes;
}

function decodePlcToAiTelemetry300(bytes) {
  return decodeTelemetryBySchema(bytes, PLC_TO_AI_TELEMETRY_300);
}

function decodePlcToAiTelemetry400(bytes) {
  return decodeTelemetryBySchema(bytes, PLC_TO_AI_TELEMETRY_400);
}

function encodePlcToAiTelemetry300(dataObj) {
  return encodeTelemetryBySchema(dataObj, PLC_TO_AI_TELEMETRY_300);
}

function encodePlcToAiTelemetry400(dataObj) {
  return encodeTelemetryBySchema(dataObj, PLC_TO_AI_TELEMETRY_400);
}

module.exports = {
  readInt16LE,
  writeInt16LE,
  decodeAiToPlcSwitch,
  encodeAiToPlcSwitch,
  decodePlcToAiStatus,
  encodeAiToPlcAnalog,
  decodeAiToPlcAnalog,
  decodePlcToAiTelemetry300,
  decodePlcToAiTelemetry400,
  encodePlcToAiTelemetry300,
  encodePlcToAiTelemetry400,
};
