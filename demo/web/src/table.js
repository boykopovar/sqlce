function columnNamesFromSchema(schema, rows) {
  if (schema.length > 0) {
    return schema.slice().sort((a, b) => a.ordinal - b.ordinal).map((column) => column.name);
  }
  if (rows.length > 0) {
    return Object.keys(rows[0]);
  }
  return [];
}

function renderRowLimitSelect(totalRows) {
  el.rowLimitSelect.innerHTML = "";
  if (totalRows <= DEFAULT_ROW_LIMIT) {
    el.rowLimitRow.hidden = true;
    state.rowLimit = totalRows;
    return;
  }
  el.rowLimitRow.hidden = false;
  for (const limit of ROW_LIMIT_OPTIONS) {
    const option = document.createElement("option");
    option.value = String(limit);
    option.textContent = String(limit);
    el.rowLimitSelect.appendChild(option);
  }
  const allOption = document.createElement("option");
  allOption.value = "all";
  allOption.textContent = t("rowLimit.all");
  el.rowLimitSelect.appendChild(allOption);
  el.rowLimitSelect.options[0].selected = true;
  state.rowLimit = DEFAULT_ROW_LIMIT;
}

function displayedRows() {
  if (state.rowLimit === "all" || state.activeRows.length <= state.rowLimit) {
    return state.activeRows;
  }
  return state.activeRows.slice(0, state.rowLimit);
}

function cellTextValue(cell) {
  if (!cell || cell.isNull) {
    return "NULL";
  }
  return String(cell.value);
}

function columnsNeedingTruncation(columnNames, rows) {
  const longColumns = new Set();
  for (const columnName of columnNames) {
    for (const row of rows) {
      const text = cellTextValue(row[columnName]);
      if (text.length > CELL_TRUNCATE_LENGTH) {
        longColumns.add(columnName);
        break;
      }
    }
  }
  return longColumns;
}

function expandColumn(columnName) {
  state.expandedColumns.add(columnName);
  renderTable(state.activeSchema, displayedRows(), state.activeTable);
}

function buildCellContent(td, columnName, text, isTruncatable) {
  if (!isTruncatable || state.expandedColumns.has(columnName) || text.length <= CELL_TRUNCATE_LENGTH) {
    td.textContent = text;
    return;
  }
  const visiblePart = text.slice(0, CELL_TRUNCATE_LENGTH);
  td.appendChild(document.createTextNode(visiblePart));
  const ellipsis = document.createElement("button");
  ellipsis.type = "button";
  ellipsis.className = "cell-ellipsis";
  ellipsis.textContent = "…";
  ellipsis.title = t("table.expandColumn");
  ellipsis.addEventListener("click", () => expandColumn(columnName));
  td.appendChild(ellipsis);
}

function renderTable(schema, rows, tableName) {
  const columnNames = columnNamesFromSchema(schema, rows);
  const truncatableColumns = columnsNeedingTruncation(columnNames, rows);

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
      const isTruncatable = truncatableColumns.has(columnName);
      if (!cell || cell.isNull) {
        td.classList.add("is-null");
        buildCellContent(td, columnName, "NULL", isTruncatable);
      } else {
        buildCellContent(td, columnName, String(cell.value), isTruncatable);
      }
      tr.appendChild(td);
    }
    fragment.appendChild(tr);
  }
  el.dataTbody.appendChild(fragment);

  el.dataPanel.hidden = false;
}
