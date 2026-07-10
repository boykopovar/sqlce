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
  tablesPanel: document.getElementById("tablesPanel"),
  tableList: document.getElementById("tableList"),
  dataPanel: document.getElementById("dataPanel"),
  dataTitle: document.getElementById("dataTitle"),
  dataThead: document.getElementById("dataThead"),
  dataTbody: document.getElementById("dataTbody"),
  exportBtn: document.getElementById("exportBtn"),
};

function setStatus(message, isError) {
  el.status.textContent = message || "";
  el.status.classList.toggle("is-error", Boolean(isError));
}

function resetResultPanels() {
  el.tablesPanel.hidden = true;
  el.dataPanel.hidden = true;
  el.tableList.innerHTML = "";
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

function renderTableList() {
  el.tableList.innerHTML = "";
  for (const tableName of state.tables) {
    const li = document.createElement("li");
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = tableName;
    button.addEventListener("click", () => selectTable(tableName));
    li.appendChild(button);
    el.tableList.appendChild(li);
  }
  el.tablesPanel.hidden = state.tables.length === 0;
}

function markActiveTableButton(tableName) {
  const buttons = el.tableList.querySelectorAll("button");
  buttons.forEach((button) => {
    button.classList.toggle("is-active", button.textContent === tableName);
  });
}

async function selectTable(tableName) {
  try {
    markActiveTableButton(tableName);
    setStatus("Чтение таблицы " + tableName + "…", false);
    const rawSchemaJson = state.module.SdfDatabase.tableSchemaJson(state.handle, tableName);
    const schema = parseDataResult(rawSchemaJson);

    const rawDataJson = state.module.SdfDatabase.tableDataJson(state.handle, tableName);
    const rows = parseDataResult(rawDataJson);

    state.activeTable = tableName;
    state.activeRows = rows;
    renderTable(schema, rows, tableName);
    setStatus("Таблица " + tableName + ": " + rows.length + " строк.", false);
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

  el.dataTitle.textContent = "Данные: " + tableName;
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

async function handleFile(file) {
  resetResultPanels();
  closeCurrentHandle();
  state.fileName = file.name;
  el.dropzoneHint.textContent = file.name;
  el.passwordRow.hidden = true;

  const arrayBuffer = await file.arrayBuffer();
  const bytes = new Uint8Array(arrayBuffer);

  try {
    setStatus("Открытие " + file.name + "…", false);
    const { module, handle } = await openDatabase(bytes, null);
    state.module = module;
    state.handle = handle;
    await loadTableList();
    renderTableList();
    setStatus("Готово. Таблиц: " + state.tables.length + ".", false);
  } catch (error) {
    if (String(error.message || "").toLowerCase().includes("password")) {
      el.passwordRow.hidden = false;
      state.pendingBytes = bytes;
      setStatus("База зашифрована паролем. Введите пароль.", false);
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
    setStatus("Проверка пароля…", false);
    const { module, handle } = await openDatabase(state.pendingBytes, password);
    state.module = module;
    state.handle = handle;
    await loadTableList();
    renderTableList();
    setStatus("Готово. Таблиц: " + state.tables.length + ".", false);
  } catch (error) {
    setStatus(String(error.message || error), true);
  }
}

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

el.unlockBtn.addEventListener("click", handleUnlock);
el.exportBtn.addEventListener("click", exportActiveTableAsCsv);
