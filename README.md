# Seth Kim Website

This repository contains a static website for GitHub Pages. The home page links to `about.html` and `projects.html`; all pages share `site.css`.

## reCAPTCHA demo

`contact.html` displays a standalone reCAPTCHA checkbox. It is not connected to a form or server-side verification, so it does not protect submissions.

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