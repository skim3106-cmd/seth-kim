const CACHE_NAME = "seth-kim-site-v1";
const APP_FILES = [
  "./",
  "./index.html",
  "./about.html",
  "./projects.html",
  "./games.html",
  "./pizza-games.html",
  "./pizza-play.html",
  "./offline.html",
  "./site.css",
  "./offline.js",
  "./pizza-games.js",
  "./pizza-play.js",
  "./pizza-games.json"
];
const NETWORK_TIMEOUT_MS = 3000;

const fetchWithTimeout = (request) => {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), NETWORK_TIMEOUT_MS);

  return fetch(request, { signal: controller.signal })
    .finally(() => clearTimeout(timeout));
};

self.addEventListener("install", (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then((cache) => cache.addAll(APP_FILES))
      .then(() => self.skipWaiting())
  );
});

self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches.keys()
      .then((keys) => Promise.all(
        keys.filter((key) => key !== CACHE_NAME).map((key) => caches.delete(key))
      ))
      .then(() => self.clients.claim())
  );
});

self.addEventListener("fetch", (event) => {
  const request = event.request;
  const requestUrl = new URL(request.url);

  if (request.method !== "GET" || requestUrl.origin !== self.location.origin) {
    return;
  }

  event.respondWith(
    (request.mode === "navigate" ? fetchWithTimeout(request) : fetch(request))
      .then((response) => {
        if (response.ok) {
          const responseCopy = response.clone();
          return caches.open(CACHE_NAME)
            .then((cache) => cache.put(request, responseCopy))
            .then(() => response);
        }

        return response;
      })
      .catch(() => caches.match(request, { ignoreSearch: true })
        .then((cached) => cached || (
          request.mode === "navigate"
            ? caches.match("./offline.html")
            : Response.error()
        )))
  );
});