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
  const rawSchemaJson = state.module.SqlceDatabase.tableSchemaJson(state.handle, state.activeTable);
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
