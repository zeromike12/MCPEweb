const CACHE_NAME = 'minecraft-web-v2';

self.addEventListener('install', event => {
    self.skipWaiting();
});

self.addEventListener('activate', event => {
    event.waitUntil(
        caches.keys().then(keys => Promise.all(keys.map(k => caches.delete(k))))
            .then(() => self.clients.claim())
    );
});

self.addEventListener('fetch', event => {
    // Network-only / direct fetch to avoid stale cache issues in development/live preview
    event.respondWith(fetch(event.request));
});
