(() => {
  const embedded = window.parent !== window;
  const params = new URLSearchParams(location.search);
  const requested = new Set(
    (params.get("sections") || "")
      .split(",")
      .map((value) => value.trim())
      .filter(Boolean)
  );

  function sectionId(section) {
    return section.querySelector("h2[id]")?.id || "";
  }

  function applySectionFilter() {
    if (!requested.size) return;
    for (const section of document.querySelectorAll("main.module-content > section")) {
      section.hidden = !requested.has(sectionId(section));
    }
  }

  function buildStandaloneNavigation() {
    const nav = document.querySelector(".module-nav");
    if (!nav) return;
    nav.replaceChildren();

    const group = document.createElement("div");
    group.className = "nav-group";
    group.textContent = document.body.dataset.moduleLabel || "实施章节";
    nav.append(group);

    for (const section of document.querySelectorAll(
      "main.module-content > section:not([hidden])"
    )) {
      const heading = section.querySelector(":scope > h2[id]");
      if (!heading) continue;

      const details = document.createElement("details");
      details.className = "nav-tree";
      const summary = document.createElement("summary");
      summary.textContent = heading.textContent;
      summary.dataset.target = heading.id;
      const links = document.createElement("div");
      links.className = "sub-links";

      const overview = document.createElement("a");
      overview.href = "#" + heading.id;
      overview.textContent = "章节概览";
      links.append(overview);

      for (const child of section.querySelectorAll("h3[id]")) {
        const link = document.createElement("a");
        link.href = "#" + child.id;
        link.textContent = child.textContent;
        links.append(link);
      }

      details.append(summary, links);
      nav.append(details);
    }
  }

  function visibleAnchors() {
    const result = [];
    for (const element of document.querySelectorAll("[id]")) {
      const section = element.closest("section");
      if ((section && section.hidden) || element.closest(".module-only")) continue;
      result.push({
        id: element.id,
        top: element.getBoundingClientRect().top + window.scrollY,
      });
    }
    return result;
  }

  let reportQueued = false;
  function reportLayout() {
    if (!embedded || reportQueued) return;
    reportQueued = true;
    requestAnimationFrame(() => {
      reportQueued = false;
      const height = Math.max(
        document.body.scrollHeight,
        document.documentElement.scrollHeight
      );
      window.parent.postMessage(
        {
          type: "shader-implementation-layout",
          height,
          anchors: visibleAnchors(),
        },
        "*"
      );
    });
  }

  function initialize() {
    document.documentElement.classList.toggle("embedded", embedded);
    applySectionFilter();
    buildStandaloneNavigation();

    document.addEventListener("click", (event) => {
      const link = event.target.closest('a[href^="#"]');
      const summary = event.target.closest("details.nav-tree > summary");
      const rawId = link
        ? link.getAttribute("href").slice(1)
        : summary?.dataset.target || "";
      if (!rawId) return;

      let id;
      try {
        id = decodeURIComponent(rawId);
      } catch {
        id = rawId;
      }
      if (!id) return;

      if (embedded) {
        event.preventDefault();
        window.parent.postMessage(
          { type: "shader-implementation-navigate", id },
          "*"
        );
        return;
      }

      if (summary) {
        requestAnimationFrame(() => {
          document.getElementById(id)?.scrollIntoView({
            behavior: "smooth",
            block: "start",
          });
          const next = "#" + encodeURIComponent(id);
          if (location.hash !== next) history.pushState(null, "", next);
        });
      }
    });

    if (embedded) {
      if ("ResizeObserver" in window) {
        new ResizeObserver(reportLayout).observe(document.body);
      }
      window.addEventListener("load", reportLayout);
      document.fonts?.ready.then(reportLayout);
      reportLayout();
    }
  }

  window.addEventListener("message", (event) => {
    if (event.data?.type === "shader-implementation-request-layout") {
      reportLayout();
      return;
    }

    if (
      event.data?.type === "shader-implementation-anchor-request" &&
      typeof event.data.id === "string"
    ) {
      const target = document.getElementById(event.data.id);
      const section = target?.closest("section");
      if (!target || section?.hidden) return;
      window.parent.postMessage(
        {
          type: "shader-implementation-anchor",
          id: event.data.id,
          top: target.getBoundingClientRect().top + window.scrollY,
        },
        "*"
      );
    }
  });

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initialize, { once: true });
  } else {
    initialize();
  }
})();
