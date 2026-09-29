(function () {
  const menuButton = document.querySelector('.menu-toggle');
  const navigation = document.querySelector('.primary-nav');

  function closeNavigation() {
    if (!menuButton || !navigation) return;
    menuButton.setAttribute('aria-expanded', 'false');
    menuButton.setAttribute('aria-label', 'Open navigation');
    navigation.classList.remove('open');
  }

  if (menuButton && navigation) {
    menuButton.addEventListener('click', function () {
      const isOpen = menuButton.getAttribute('aria-expanded') !== 'true';
      menuButton.setAttribute('aria-expanded', String(isOpen));
      menuButton.setAttribute('aria-label', isOpen ? 'Close navigation' : 'Open navigation');
      navigation.classList.toggle('open', isOpen);
    });
    navigation.querySelectorAll('a').forEach(function (link) {
      link.addEventListener('click', closeNavigation);
    });
    document.addEventListener('keydown', function (event) {
      if (event.key === 'Escape') closeNavigation();
    });
  }

  const status = document.querySelector('.copy-status');
  document.querySelectorAll('[data-copy]').forEach(function (button) {
    button.addEventListener('click', async function () {
      const target = document.getElementById(button.dataset.copy);
      if (!target || !status) return;
      try {
        await navigator.clipboard.writeText(target.textContent);
        button.textContent = 'Copied';
        status.textContent = 'C++ example copied to clipboard.';
      } catch (_) {
        button.textContent = 'Unavailable';
        status.textContent = 'Clipboard access is unavailable in this browser context.';
      }
      window.setTimeout(function () { button.textContent = 'Copy'; }, 1500);
    });
  });
})();