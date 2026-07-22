function setStatus(message, isError) {
  el.status.textContent = message || "";
  el.status.classList.toggle("is-error", Boolean(isError));
}
