const updateOfflineNotice = () => {
  let notice = document.querySelector(".offline-notice");

  if (!notice) {
    notice = document.createElement("div");
    notice.className = "offline-notice";
    notice.setAttribute("role", "status");
    notice.setAttribute("aria-live", "polite");
    notice.textContent = "Offline: bundled games are available; external games need internet.";
    document.body.append(notice);
  }

  notice.hidden = navigator.onLine;
};

window.addEventListener("online", updateOfflineNotice);
window.addEventListener("offline", updateOfflineNotice);
updateOfflineNotice();

if ("serviceWorker" in navigator) {
  navigator.serviceWorker.register("./sw.js").catch(() => {});
}