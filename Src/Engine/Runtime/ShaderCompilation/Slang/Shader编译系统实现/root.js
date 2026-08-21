(() => {
  const frames = Array.from(document.querySelectorAll("iframe.module-frame"));
  const frameState = new Map();

  function decodeFragment(value) {
    try {
      return decodeURIComponent(value);
    } catch {
      return value;
    }
  }

  let pendingAnchor = decodeFragment(location.hash.slice(1));

  function frameForSource(source) {
    return frames.find((frame) => frame.contentWindow === source) || null;
  }

  function updateHash(id) {
    const next = "#" + encodeURIComponent(id);
    if (location.hash !== next) {
      history.pushState(null, "", next);
    }
  }

  function frameOwnsAnchor(frame, id) {
    return (frame.dataset.anchors || "")
      .split(",")
      .some((anchor) => anchor.trim() === id);
  }

  function scrollToFramePosition(frame, top, instant) {
    const frameTop = window.scrollY + frame.getBoundingClientRect().top;
    window.scrollTo({
      top: Math.max(0, frameTop + top - 20),
      behavior: instant ? "auto" : "smooth",
    });
  }

  function navigateTo(id, options = {}) {
    if (!id) return false;

    const local = document.getElementById(id);
    if (local) {
      local.scrollIntoView({
        behavior: options.instant ? "auto" : "smooth",
        block: "start",
      });
      if (options.updateHash !== false) updateHash(id);
      pendingAnchor = "";
      return true;
    }

    for (const [frame, state] of frameState) {
      const anchor = state.anchors.get(id);
      if (anchor == null) continue;
      scrollToFramePosition(frame, anchor, options.instant);
      if (options.updateHash !== false) updateHash(id);
      pendingAnchor = "";
      return true;
    }

    for (const frame of frames) {
      if (!frameOwnsAnchor(frame, id)) continue;
      scrollToFramePosition(frame, 0, options.instant);
      frame.contentWindow?.postMessage(
        { type: "shader-implementation-anchor-request", id },
        "*"
      );
      if (options.updateHash !== false) updateHash(id);
      pendingAnchor = id;
      return true;
    }

    pendingAnchor = id;
    return false;
  }

  window.addEventListener("message", (event) => {
    const frame = frameForSource(event.source);
    const data = event.data;
    if (!frame || !data || typeof data !== "object") return;

    if (data.type === "shader-implementation-layout") {
      const height = Math.max(120, Math.min(240000, Math.ceil(Number(data.height) || 0)));
      frame.style.height = height + "px";
      const anchors = new Map();
      for (const item of Array.isArray(data.anchors) ? data.anchors : []) {
        if (item && typeof item.id === "string" && Number.isFinite(item.top)) {
          anchors.set(item.id, item.top);
        }
      }
      frameState.set(frame, { anchors });
      if (pendingAnchor) {
        requestAnimationFrame(() =>
          navigateTo(pendingAnchor, { instant: true, updateHash: false })
        );
      }
      return;
    }

    if (
      data.type === "shader-implementation-anchor" &&
      typeof data.id === "string" &&
      Number.isFinite(data.top)
    ) {
      const state = frameState.get(frame) || { anchors: new Map() };
      state.anchors.set(data.id, data.top);
      frameState.set(frame, state);
      if (pendingAnchor === data.id) {
        navigateTo(data.id, { instant: true, updateHash: false });
      }
      return;
    }

    if (
      data.type === "shader-implementation-navigate" &&
      typeof data.id === "string"
    ) {
      navigateTo(data.id);
    }
  });

  document.addEventListener("click", (event) => {
    const link = event.target.closest('a[href^="#"]');
    if (link) {
      const id = decodeFragment(link.getAttribute("href").slice(1));
      if (!id) return;
      event.preventDefault();
      navigateTo(id);
      return;
    }

    const summary = event.target.closest("details.nav-tree > summary");
    if (!summary) return;
    const overview = summary.parentElement.querySelector(
      ':scope > .sub-links a[href^="#"]'
    );
    if (!overview) return;
    const id = decodeFragment(overview.getAttribute("href").slice(1));
    if (id) requestAnimationFrame(() => navigateTo(id));
  });

  window.addEventListener("hashchange", () => {
    navigateTo(decodeFragment(location.hash.slice(1)), {
      instant: true,
      updateHash: false,
    });
  });
  window.addEventListener("popstate", () => {
    navigateTo(decodeFragment(location.hash.slice(1)), {
      instant: true,
      updateHash: false,
    });
  });

  for (const frame of frames) {
    const requestLayout = () => {
      frame.contentWindow?.postMessage(
        { type: "shader-implementation-request-layout" },
        "*"
      );
    };
    frame.addEventListener("load", requestLayout);
    requestAnimationFrame(requestLayout);
  }

  if (pendingAnchor) {
    navigateTo(pendingAnchor, { instant: true, updateHash: false });
  }
})();
