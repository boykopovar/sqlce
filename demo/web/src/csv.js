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

function sanitizeZipEntryName(name) {
  return String(name).replace(/[\\/:*?"<>|]/g, "_");
}

function buildCsvBytesForTable(tableName) {
  const rawSchemaJson = state.module.SqlceDatabase.tableSchemaJson(state.handle, tableName);
  const schema = parseDataResult(rawSchemaJson);
  const rawDataJson = state.module.SqlceDatabase.tableDataJson(state.handle, tableName);
  const rows = parseDataResult(rawDataJson);
  const csv = "sep=,\r\n" + buildCsv(schema, rows);
  return new TextEncoder().encode("\ufeff" + csv);
}

function downloadBlob(blob, fileName) {
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = fileName;
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
  URL.revokeObjectURL(url);
}

function exportAllTablesAsCsvZip() {
  if (!state.module || !state.handle || state.tables.length === 0) {
    return;
  }
  try {
    const usedNames = new Set();
    const entries = state.tables.map((tableName) => {
      let entryName = sanitizeZipEntryName(tableName) + ".csv";
      let suffix = 2;
      while (usedNames.has(entryName.toLowerCase())) {
        entryName = sanitizeZipEntryName(tableName) + "_" + suffix + ".csv";
        suffix += 1;
      }
      usedNames.add(entryName.toLowerCase());
      return { name: entryName, data: buildCsvBytesForTable(tableName) };
    });
    const zipBlob = buildZipStore(entries);
    const baseName = (state.fileName || "sqlce").replace(/\.sdf$/i, "");
    downloadBlob(zipBlob, baseName + "_csv.zip");
  } catch (error) {
    setStatus(String(error.message || error), true);
  }
}
