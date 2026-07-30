const ZIP_CRC_TABLE = (() => {
  const table = new Uint32Array(256);
  for (let i = 0; i < 256; i++) {
    let c = i;
    for (let k = 0; k < 8; k++) {
      c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    }
    table[i] = c >>> 0;
  }
  return table;
})();

function zipCrc32(bytes) {
  let crc = 0xffffffff;
  for (let i = 0; i < bytes.length; i++) {
    crc = ZIP_CRC_TABLE[(crc ^ bytes[i]) & 0xff] ^ (crc >>> 8);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function zipDosDateTime(date) {
  const time = ((date.getHours() & 0x1f) << 11) | ((date.getMinutes() & 0x3f) << 5) | ((date.getSeconds() >> 1) & 0x1f);
  const dosYear = Math.max(0, date.getFullYear() - 1980);
  const dateValue = ((dosYear & 0x7f) << 9) | (((date.getMonth() + 1) & 0xf) << 5) | (date.getDate() & 0x1f);
  return { time, date: dateValue };
}

function zipWriteUint16(view, offset, value) {
  view.setUint16(offset, value, true);
}

function zipWriteUint32(view, offset, value) {
  view.setUint32(offset, value, true);
}

function buildZipStore(entries) {
  const encoder = new TextEncoder();
  const now = new Date();
  const { time: dosTime, date: dosDate } = zipDosDateTime(now);

  const localParts = [];
  const centralParts = [];
  let offset = 0;

  for (const entry of entries) {
    const nameBytes = encoder.encode(entry.name);
    const dataBytes = entry.data;
    const crc = zipCrc32(dataBytes);
    const size = dataBytes.length;

    const localHeader = new ArrayBuffer(30);
    const localView = new DataView(localHeader);
    zipWriteUint32(localView, 0, 0x04034b50);
    zipWriteUint16(localView, 4, 20);
    zipWriteUint16(localView, 6, 0x0800);
    zipWriteUint16(localView, 8, 0);
    zipWriteUint16(localView, 10, dosTime);
    zipWriteUint16(localView, 12, dosDate);
    zipWriteUint32(localView, 14, crc);
    zipWriteUint32(localView, 18, size);
    zipWriteUint32(localView, 22, size);
    zipWriteUint16(localView, 26, nameBytes.length);
    zipWriteUint16(localView, 28, 0);

    localParts.push(new Uint8Array(localHeader), nameBytes, dataBytes);

    const centralHeader = new ArrayBuffer(46);
    const centralView = new DataView(centralHeader);
    zipWriteUint32(centralView, 0, 0x02014b50);
    zipWriteUint16(centralView, 4, 20);
    zipWriteUint16(centralView, 6, 20);
    zipWriteUint16(centralView, 8, 0x0800);
    zipWriteUint16(centralView, 10, 0);
    zipWriteUint16(centralView, 12, dosTime);
    zipWriteUint16(centralView, 14, dosDate);
    zipWriteUint32(centralView, 16, crc);
    zipWriteUint32(centralView, 20, size);
    zipWriteUint32(centralView, 24, size);
    zipWriteUint16(centralView, 28, nameBytes.length);
    zipWriteUint16(centralView, 30, 0);
    zipWriteUint16(centralView, 32, 0);
    zipWriteUint16(centralView, 34, 0);
    zipWriteUint16(centralView, 36, 0);
    zipWriteUint32(centralView, 38, 0);
    zipWriteUint32(centralView, 42, offset);

    centralParts.push(new Uint8Array(centralHeader), nameBytes);

    offset += localHeader.byteLength + nameBytes.length + size;
  }

  const centralStart = offset;
  let centralSize = 0;
  for (const part of centralParts) {
    centralSize += part.length;
  }

  const endRecord = new ArrayBuffer(22);
  const endView = new DataView(endRecord);
  zipWriteUint32(endView, 0, 0x06054b50);
  zipWriteUint16(endView, 4, 0);
  zipWriteUint16(endView, 6, 0);
  zipWriteUint16(endView, 8, entries.length);
  zipWriteUint16(endView, 10, entries.length);
  zipWriteUint32(endView, 12, centralSize);
  zipWriteUint32(endView, 16, centralStart);
  zipWriteUint16(endView, 20, 0);

  return new Blob([...localParts, ...centralParts, new Uint8Array(endRecord)], { type: "application/zip" });
}
