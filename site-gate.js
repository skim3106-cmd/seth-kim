(() => {
  const accessKey = 'sethKimCaptchaAccess';

  if (!navigator.onLine) {
    return;
  }

  try {
    if (sessionStorage.getItem(accessKey) === 'granted') {
      return;
    }
  } catch {
    // Keep the gate usable when browser storage is unavailable.
  }

  const pageElements = Array.from(document.body.children);
  pageElements.forEach((element) => {
    element.inert = true;
  });

  const gate = document.createElement('main');
  gate.className = 'captcha-gate';
  gate.setAttribute('role', 'dialog');
  gate.setAttribute('aria-modal', 'true');
  gate.setAttribute('aria-labelledby', 'captcha-gate-title');

  const content = document.createElement('div');
  content.className = 'captcha-gate-content';

  const eyebrow = document.createElement('p');
  eyebrow.className = 'eyebrow';
  eyebrow.textContent = 'THE WEBSPACE OF SETH KIM';

  const heading = document.createElement('h1');
  heading.id = 'captcha-gate-title';
  heading.tabIndex = -1;
  heading.append('One quick ', document.createElement('br'));
  const headingAccent = document.createElement('span');
  headingAccent.textContent = 'check.';
  heading.append(headingAccent);

  const copy = document.createElement('p');
  copy.className = 'captcha-gate-copy';
  copy.textContent = 'Complete the checkbox to continue.';

  const widgetContainer = document.createElement('div');
  widgetContainer.className = 'recaptcha-wrap';
  const widget = document.createElement('div');
  widgetContainer.append(widget);

  const status = document.createElement('p');
  status.className = 'captcha-gate-status';
  status.setAttribute('role', 'status');
  status.setAttribute('aria-live', 'polite');

  const note = document.createElement('p');
  note.className = 'captcha-gate-note';
  note.textContent = 'This browser-only check does not provide server-side protection.';

  content.append(eyebrow, heading, copy, widgetContainer, status, note);
  gate.append(content);
  document.body.append(gate);
  heading.focus();

  const continueToSite = () => {
    try {
      sessionStorage.setItem(accessKey, 'granted');
    } catch {
      // The current page can still continue if storage is unavailable.
    }

    pageElements.forEach((element) => {
      element.inert = false;
    });
    gate.remove();
  };

  const offerOfflineAccess = () => {
    status.textContent = 'Verification is unavailable. You can continue to cached pages.';
    if (content.querySelector('.captcha-offline-continue')) return;

    const continueButton = document.createElement('button');
    continueButton.className = 'captcha-offline-continue';
    continueButton.type = 'button';
    continueButton.textContent = 'Continue to saved pages';
    continueButton.addEventListener('click', continueToSite, { once: true });
    status.after(continueButton);
  };

  const renderCheckbox = () => {
    try {
      window.grecaptcha.render(widget, {
        sitekey: '6LeUdNgtAAAAAJLlaMrFpZNwgFMGO7ydiCZ3BvV6',
        callback: continueToSite,
        'error-callback': offerOfflineAccess,
        'expired-callback': () => {
          status.textContent = 'The check expired. Please complete it again.';
        },
      });
    } catch {
      offerOfflineAccess();
    }
  };

  window.siteGateRecaptchaLoaded = renderCheckbox;
  const recaptchaScript = document.createElement('script');
  recaptchaScript.src = 'https://www.google.com/recaptcha/api.js?onload=siteGateRecaptchaLoaded&render=explicit';
  recaptchaScript.async = true;
  recaptchaScript.defer = true;
  recaptchaScript.onerror = offerOfflineAccess;
  document.head.append(recaptchaScript);
})();