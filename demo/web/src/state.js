const ROW_LIMIT_OPTIONS = [100, 1000];
const DEFAULT_ROW_LIMIT = 100;
const CELL_TRUNCATE_LENGTH = 60;
const CELL_TRUNCATE_MIN_LENGTH = 60;
const CELL_TRUNCATE_MAX_LENGTH = 2000;
const CELL_TRUNCATE_GROW_STEP = 40;

const state = {
  module: null,
  handle: null,
  fileName: null,
  pendingBytes: null,
  tables: [],
  activeTable: null,
  activeRows: [],
  activeSchema: [],
  rowLimit: DEFAULT_ROW_LIMIT,
  openedWithPassword: false,
  expandedColumns: new Set(),
  fullscreenTruncateLength: CELL_TRUNCATE_LENGTH,
};

const el = {
  dropzone: document.getElementById("dropzone"),
  dropzoneHint: document.getElementById("dropzoneHint"),
  fileInput: document.getElementById("fileInput"),
  passwordRow: document.getElementById("passwordRow"),
  passwordInput: document.getElementById("passwordInput"),
  unlockBtn: document.getElementById("unlockBtn"),
  status: document.getElementById("status"),
  fileMeta: document.getElementById("fileMeta"),
  dataPanel: document.getElementById("dataPanel"),
  tableSelect: document.getElementById("tableSelect"),
  rowLimitRow: document.getElementById("rowLimitRow"),
  rowLimitSelect: document.getElementById("rowLimitSelect"),
  dataTable: document.getElementById("dataTable"),
  dataThead: document.getElementById("dataThead"),
  dataTbody: document.getElementById("dataTbody"),
  tableScroll: document.getElementById("tableScroll"),
  exportBtn: document.getElementById("exportBtn"),
  exportDecryptedBtn: document.getElementById("exportDecryptedBtn"),
  fullscreenBtn: document.getElementById("fullscreenBtn"),
};

function resetState() {
  state.tables = [];
  state.activeTable = null;
  state.activeRows = [];
  state.activeSchema = [];
  state.rowLimit = DEFAULT_ROW_LIMIT;
  state.openedWithPassword = false;
  state.expandedColumns = new Set();
  state.fullscreenTruncateLength = CELL_TRUNCATE_LENGTH;
}
