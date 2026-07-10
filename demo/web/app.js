const state = {
  module: null,
  handle: null,
  fileName: null,
  tables: [],
  activeTable: null,
  activeRows: [],
};

const el = {
  dropzone: document.getElementById("dropzone"),
  dropzoneHint: document.getElementById("dropzoneHint"),
  fileInput: document.getElementById("fileInput"),
  passwordRow: document.getElementById("passwordRow"),
  passwordInput: document.getElementById("passwordInput"),
  unlockBtn: document.getElementById("unlockBtn"),
  status: document.getElementById("status"),
  dataPanel: document.getElementById("dataPanel"),
  tableSelect: document.getElementById("tableSelect"),
  dataThead: document.getElementById("dataThead"),
  dataTbody: document.getElementById("dataTbody"),
  exportBtn: document.getElementById("exportBtn"),
};

function setStatus(message, isError) {
  el.status.textContent = message || "";
  el.status.classList.toggle("is-error", Boolean(isError));
}

function resetResultPanels() {
  el.dataPanel.hidden = true;
  el.tableSelect.innerHTML = "";
  el.dataThead.innerHTML = "";
  el.dataTbody.innerHTML = "";
  state.tables = [];
  state.activeTable = null;
  state.activeRows = [];
}

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
    ? module.SdfDatabase.openWithPassword(path, password)
    : module.SdfDatabase.open(path);
  const handle = parseOpenResult(rawResult);
  return { module, handle };
}

function closeCurrentHandle() {
  if (state.module && state.handle) {
    state.module.SdfDatabase.close(state.handle);
  }
  state.handle = null;
}

async function loadTableList() {
  const rawJson = state.module.SdfDatabase.listTablesJson(state.handle);
  state.tables = parseDataResult(rawJson);
}

function renderTableSelect() {
  el.tableSelect.innerHTML = "";
  for (const tableName of state.tables) {
    const option = document.createElement("option");
    option.value = tableName;
    option.textContent = tableName;
    el.tableSelect.appendChild(option);
  }
  if (state.tables.length > 0) {
    el.tableSelect.value = state.tables[0];
    selectTable(state.tables[0]);
  }
}

async function selectTable(tableName) {
  try {
    el.tableSelect.value = tableName;
    setStatus(t("status.readingTable", { tableName }), false);
    const rawSchemaJson = state.module.SdfDatabase.tableSchemaJson(state.handle, tableName);
    const schema = parseDataResult(rawSchemaJson);

    const rawDataJson = state.module.SdfDatabase.tableDataJson(state.handle, tableName);
    const rows = parseDataResult(rawDataJson);

    state.activeTable = tableName;
    state.activeRows = rows;
    renderTable(schema, rows, tableName);
    setStatus(t("status.tableRows", { tableName, count: rows.length, rowWord: pluralize(rows.length, "row") }), false);
  } catch (error) {
    setStatus(String(error.message || error), true);
  }
}

function columnNamesFromSchema(schema, rows) {
  if (schema.length > 0) {
    return schema.slice().sort((a, b) => a.ordinal - b.ordinal).map((column) => column.name);
  }
  if (rows.length > 0) {
    return Object.keys(rows[0]);
  }
  return [];
}

function renderTable(schema, rows, tableName) {
  const columnNames = columnNamesFromSchema(schema, rows);

  el.dataThead.innerHTML = "";
  const headRow = document.createElement("tr");
  for (const columnName of columnNames) {
    const th = document.createElement("th");
    th.textContent = columnName;
    headRow.appendChild(th);
  }
  el.dataThead.appendChild(headRow);

  el.dataTbody.innerHTML = "";
  const fragment = document.createDocumentFragment();
  for (const row of rows) {
    const tr = document.createElement("tr");
    for (const columnName of columnNames) {
      const cell = row[columnName];
      const td = document.createElement("td");
      if (!cell || cell.isNull) {
        td.textContent = "NULL";
        td.classList.add("is-null");
      } else {
        td.textContent = cell.value;
      }
      tr.appendChild(td);
    }
    fragment.appendChild(tr);
  }
  el.dataTbody.appendChild(fragment);

  el.dataPanel.hidden = false;
}

function csvEscape(value) {
  const needsQuoting = /[",\n]/.test(value);
  const escaped = value.replace(/"/g, "\"\"");
  return needsQuoting ? "\"" + escaped + "\"" : escaped;
}

function buildCsv(schema, rows) {
  const columnNames = columnNamesFromSchema(schema, rows);
  const lines = [columnNames.map(csvEscape).join(",")];
  for (const row of rows) {
    const cells = columnNames.map((columnName) => {
      const cell = row[columnName];
      if (!cell || cell.isNull) {
        return "";
      }
      return csvEscape(cell.value);
    });
    lines.push(cells.join(","));
  }
  return lines.join("\n");
}

function exportActiveTableAsCsv() {
  if (!state.activeTable) {
    return;
  }
  const rawSchemaJson = state.module.SdfDatabase.tableSchemaJson(state.handle, state.activeTable);
  const schema = parseDataResult(rawSchemaJson);
  const csv = "sep=,\r\n" + buildCsv(schema, state.activeRows);

  const blob = new Blob(["\ufeff", csv], { type: "text/csv;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = state.activeTable + ".csv";
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
  URL.revokeObjectURL(url);
}

function encryptionModeName(mode) {
  const names = ["NONE", "RC4_SHA1", "AES128_SHA1", "AES128_SHA256", "AES256_SHA512"];
  return names[mode] !== undefined ? names[mode] : String(mode);
}

async function handleFile(file) {
  resetResultPanels();
  closeCurrentHandle();
  state.fileName = file.name;
  el.dropzoneHint.textContent = file.name;
  el.passwordRow.hidden = true;

  const arrayBuffer = await file.arrayBuffer();
  const bytes = new Uint8Array(arrayBuffer);

  try {
    setStatus(t("status.opening", { fileName: file.name }), false);
    const { module, handle } = await openDatabase(bytes, null);
    state.module = module;
    state.handle = handle;
    await loadTableList();
    renderTableSelect();
    setStatus(t("status.done", { count: state.tables.length, tableWord: pluralize(state.tables.length, "table") }), false);
  } catch (error) {
    if (String(error.message || "").toLowerCase().includes("password")) {
      el.passwordRow.hidden = false;
      state.pendingBytes = bytes;
      const module = await ensureModule();
      const path = writeFileToVirtualFs(module, bytes);
      const modeRawJson = module.SdfDatabase.encryptionModeOfFile(path);
      const mode = parseDataResult(modeRawJson);
      setStatus(t("status.passwordRequired", { algorithm: encryptionModeName(mode) }), false);
    } else {
      setStatus(String(error.message || error), true);
    }
  }
}

async function handleUnlock() {
  if (!state.pendingBytes) {
    return;
  }
  const password = el.passwordInput.value;
  try {
    setStatus(t("status.checkingPassword"), false);
    const { module, handle } = await openDatabase(state.pendingBytes, password);
    state.module = module;
    state.handle = handle;
    await loadTableList();
    renderTableSelect();
    setStatus(t("status.done", { count: state.tables.length, tableWord: pluralize(state.tables.length, "table") }), false);
  } catch (error) {
    setStatus(String(error.message || error), true);
  }
}

(async () => {
  await i18nReady;

  el.fileInput.addEventListener("change", (event) => {
    const file = event.target.files && event.target.files[0];
    if (file) {
      handleFile(file);
    }
  });

  el.dropzone.addEventListener("dragover", (event) => {
    event.preventDefault();
    el.dropzone.classList.add("is-dragover");
  });

  el.dropzone.addEventListener("dragleave", () => {
    el.dropzone.classList.remove("is-dragover");
  });

  el.dropzone.addEventListener("drop", (event) => {
    event.preventDefault();
    el.dropzone.classList.remove("is-dragover");
    const file = event.dataTransfer.files && event.dataTransfer.files[0];
    if (file) {
      handleFile(file);
    }
  });

  el.tableSelect.addEventListener("change", (event) => {
    selectTable(event.target.value);
  });

  el.unlockBtn.addEventListener("click", handleUnlock);
  el.exportBtn.addEventListener("click", exportActiveTableAsCsv);
})();
