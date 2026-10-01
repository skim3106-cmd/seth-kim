# Seth Kim Website

This repository contains a static website for GitHub Pages. The home page links to `about.html` and `projects.html`; all pages share `site.css`.

## Site entry checkbox

The Google reCAPTCHA checkbox appears before regular site pages. Completing it
continues to the requested page and remembers access for the current browser
tab. This is a client-side gate only; it does not provide server-side
verification or protect private content.

## Offline mode

The service worker caches the site's pages, styles, scripts, Pizza Edition
game list, and locally bundled 2048, Snake, and Tic-Tac-Toe games after the
site is first opened online. Offline support requires a browser with service
worker support and an HTTPS connection (GitHub Pages provides HTTPS). If the
connection is lost, cached pages and bundled games remain available and the
checkbox gate is skipped. Games hosted by Pizza Edition and other third-party
content still require internet access.

## Push changes

After making changes, commit and push them with:

```bash
git add .
git commit -m "Update website"
git push origin main
```

## Publish with GitHub Pages

1. Push this repository to GitHub.
2. In the repository, open **Settings → Pages**.
3. Under **Build and deployment**, choose **Deploy from a branch**.
4. Select the `main` branch and `/ (root)`, then save.

GitHub Pages will publish the site at `https://<your-username>.github.io/<repository-name>/`. Add more `.html` files in the repository root and link to them from `index.html` and the navigation menus.