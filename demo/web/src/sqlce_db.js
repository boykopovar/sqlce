async function ensureModule() {
  if (state.module) {
    return state.module;
  }
  state.module = await createSqlceModule();
  return state.module;
}

function writeFileToVirtualFs(module, bytes) {
  const dir = "/work";
  try {
    module.FS.mkdir(dir);
  } catch (error) {
    if (!module.FS.analyzePath(dir).exists) {
      throw error;
    }
  }
  const path = dir + "/db.sdf";
  module.FS.writeFile(path, bytes);
  return path;
}

function parseOpenResult(rawJson) {
  const parsed = JSON.parse(rawJson);
  if (!parsed.ok) {
    throw new Error(parsed.error);
  }
  return parsed.handle;
}

function parseDataResult(rawJson) {
  const parsed = JSON.parse(rawJson);
  if (!parsed.ok) {
    throw new Error(parsed.error);
  }
  return parsed.data;
}

async function openDatabase(bytes, password) {
  const module = await ensureModule();
  const path = writeFileToVirtualFs(module, bytes);
  const rawResult = password
      ? module.SqlceDatabase.openWithPassword(path, password)
      : module.SqlceDatabase.open(path);
  const handle = parseOpenResult(rawResult);
  return { module, handle };
}

function closeCurrentHandle() {
  if (state.module && state.handle) {
    state.module.SqlceDatabase.close(state.handle);
  }
  state.handle = null;
}

async function loadTableList() {
  const rawJson = state.module.SqlceDatabase.listTablesJson(state.handle);
  state.tables = parseDataResult(rawJson);
}

function encryptionModeName(mode) {
  const names = ["NONE", "TRIPLE_DES_SHA1", "AES128_SHA1", "AES128_SHA256", "AES256_SHA512"];
  return names[mode] !== undefined ? names[mode] : String(mode);
}

function formatVersionName(version) {
  const names = {
    0: "Unknown",
    3004180: "SQL CE 3.0",
    3505053: "SQL CE 3.5",
    3505625: "SQL CE 3.5 SP2",
    4000000: "SQL CE 4.0",
  };
  return names[version] !== undefined ? names[version] : String(version);
}

function showFileFormatVersion(module, path) {
  try {
    const rawJson = module.SqlceDatabase.formatVersionOfFile(path);
    const version = parseDataResult(rawJson);
    el.fileMeta.textContent = t("file.formatVersion", { version: formatVersionName(version) });
    el.fileMeta.hidden = false;
  } catch (error) {
    el.fileMeta.hidden = true;
    el.fileMeta.textContent = "";
  }
}

function exportDecryptedDatabase() {
  if (!state.handle || !state.openedWithPassword) {
    return;
  }
  const result = state.module.SqlceDatabase.exportDecrypted(state.handle);
  if (!result.ok) {
    setStatus(String(result.error), true);
    return;
  }

  const blob = new Blob([result.data], { type: "application/octet-stream" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = (state.fileName || "db.sdf").replace(/\.sdf$/i, "") + "_decrypted.sdf";
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
  URL.revokeObjectURL(url);
}
