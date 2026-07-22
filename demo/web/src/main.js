function resetResultPanels() {
  el.dataPanel.hidden = true;
  el.tableSelect.innerHTML = "";
  el.rowLimitRow.hidden = true;
  el.rowLimitSelect.innerHTML = "";
  el.dataThead.innerHTML = "";
  el.dataTbody.innerHTML = "";
  el.fileMeta.hidden = true;
  el.fileMeta.textContent = "";
  resetState();
  el.exportDecryptedBtn.hidden = true;
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
    el.tableSelect.options[0].selected = true;
    selectTable(state.tables[0]);
  }
}

async function selectTable(tableName) {
  try {
    el.tableSelect.value = tableName;
    state.expandedColumns = new Set();
    setStatus(t("status.readingTable", { tableName }), false);
    const rawSchemaJson = state.module.SqlceDatabase.tableSchemaJson(state.handle, tableName);
    const schema = parseDataResult(rawSchemaJson);

    const rawDataJson = state.module.SqlceDatabase.tableDataJson(state.handle, tableName);
    const rows = parseDataResult(rawDataJson);

    state.activeTable = tableName;
    state.activeSchema = schema;
    state.activeRows = rows;
    renderRowLimitSelect(rows.length);
    renderTable(schema, displayedRows(), tableName);
    setStatus(t("status.tableRows", { tableName, count: rows.length, rowWord: pluralize(rows.length, "row") }), false);
  } catch (error) {
    setStatus(String(error.message || error), true);
  }
}

function handleRowLimitChange(value) {
  state.rowLimit = value === "all" ? "all" : Number(value);
  state.expandedColumns = new Set();
  renderTable(state.activeSchema, displayedRows(), state.activeTable);
}

async function handleFile(file) {
  resetResultPanels();
  closeCurrentHandle();
  state.fileName = file.name;
  el.dropzoneHint.textContent = file.name;
  el.passwordRow.hidden = true;

  const arrayBuffer = await file.arrayBuffer();
  const bytes = new Uint8Array(arrayBuffer);

  const module = await ensureModule();
  const path = writeFileToVirtualFs(module, bytes);
  showFileFormatVersion(module, path);

  try {
    setStatus(t("status.opening", { fileName: file.name }), false);
    const rawResult = module.SqlceDatabase.open(path);
    const handle = parseOpenResult(rawResult);
    state.module = module;
    state.handle = handle;
    await loadTableList();
    renderTableSelect();
    setStatus(t("status.done", { count: state.tables.length, tableWord: pluralize(state.tables.length, "table") }), false);
  } catch (error) {
    if (String(error.message || "").toLowerCase().includes("password")) {
      el.passwordRow.hidden = false;
      state.pendingBytes = bytes;
      const modeRawJson = module.SqlceDatabase.encryptionModeOfFile(path);
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
    state.openedWithPassword = true;
    el.exportDecryptedBtn.hidden = false;
    await loadTableList();
    renderTableSelect();
    const resolvedRawJson = state.module.SqlceDatabase.resolvedEncryptionModeJson(state.handle);
    const resolvedMode = parseDataResult(resolvedRawJson);
    setStatus(
        t("status.doneWithAlgorithm", {
          count: state.tables.length,
          tableWord: pluralize(state.tables.length, "table"),
          algorithm: encryptionModeName(resolvedMode),
        }),
        false);
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

  el.rowLimitSelect.addEventListener("change", (event) => {
    handleRowLimitChange(event.target.value);
  });

  el.unlockBtn.addEventListener("click", handleUnlock);
  el.exportBtn.addEventListener("click", exportActiveTableAsCsv);
  el.exportDecryptedBtn.addEventListener("click", exportDecryptedDatabase);
})();
