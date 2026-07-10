const I18N_SUPPORTED_LANGS = ["ru", "en"];
const I18N_DEFAULT_LANG = "ru";
const I18N_STORAGE_KEY = "sqlce-lang";

const i18n = {
  lang: I18N_DEFAULT_LANG,
  dict: {},
  listeners: [],
};

function detectBrowserLang() {
  const candidates = navigator.languages && navigator.languages.length ? navigator.languages : [navigator.language || ""];
  for (const candidate of candidates) {
    const short = String(candidate).slice(0, 2).toLowerCase();
    if (I18N_SUPPORTED_LANGS.includes(short)) {
      return short;
    }
  }
  return I18N_DEFAULT_LANG;
}

function resolveInitialLang() {
  const stored = localStorage.getItem(I18N_STORAGE_KEY);
  if (stored && I18N_SUPPORTED_LANGS.includes(stored)) {
    return stored;
  }
  return detectBrowserLang();
}

async function loadDict(lang) {
  const response = await fetch("i18n/" + lang + ".json");
  return response.json();
}

function applyStaticTranslations() {
  document.documentElement.lang = i18n.lang;

  document.querySelectorAll("[data-i18n]").forEach((node) => {
    const key = node.getAttribute("data-i18n");
    const value = i18n.dict[key];
    if (value !== undefined) {
      node.textContent = value;
    }
  });

  document.querySelectorAll("[data-i18n-placeholder]").forEach((node) => {
    const key = node.getAttribute("data-i18n-placeholder");
    const value = i18n.dict[key];
    if (value !== undefined) {
      node.setAttribute("placeholder", value);
    }
  });

  const trustLine = document.getElementById("trustLine");
  if (trustLine) {
    const template = i18n.dict["trust.line"] || "";
    const linkText = i18n.dict["trust.implLink"] || "";
    const linkHtml = '<a href="https://github.com/boykopovar/sqlce/tree/main/core" target="_blank" rel="noopener">' + linkText + "</a>";
    trustLine.innerHTML = template.replace("{implLink}", linkHtml);
  }

  document.querySelectorAll(".lang-btn").forEach((button) => {
    button.classList.toggle("is-active", button.dataset.lang === i18n.lang);
  });
}

function t(key, params) {
  let text = i18n.dict[key] !== undefined ? i18n.dict[key] : key;
  if (params) {
    for (const [paramKey, paramValue] of Object.entries(params)) {
      text = text.replace("{" + paramKey + "}", paramValue);
    }
  }
  return text;
}

function pluralize(count, key) {
  const mod10 = count % 10;
  const mod100 = count % 100;
  let form = "many";
  if (mod10 === 1 && mod100 !== 11) {
    form = "one";
  } else if (mod10 >= 2 && mod10 <= 4 && (mod100 < 10 || mod100 >= 20)) {
    form = "few";
  }
  return t("plural." + key + "." + form);
}

async function setLang(lang) {
  if (!I18N_SUPPORTED_LANGS.includes(lang)) {
    return;
  }
  i18n.lang = lang;
  i18n.dict = await loadDict(lang);
  localStorage.setItem(I18N_STORAGE_KEY, lang);
  applyStaticTranslations();
  for (const listener of i18n.listeners) {
    listener();
  }
}

function onLangChange(listener) {
  i18n.listeners.push(listener);
}

async function initI18n() {
  const initialLang = resolveInitialLang();
  await setLang(initialLang);

  document.querySelectorAll(".lang-btn").forEach((button) => {
    button.addEventListener("click", () => {
      setLang(button.dataset.lang);
    });
  });
}

const i18nReady = initI18n();
