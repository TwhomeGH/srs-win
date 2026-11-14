import React from 'react';

// Poll an API loader while the tab is visible. Keeps the previous value on
// transient failures so a blip does not blank the table.
export function usePolling(loader, intervalMs = 3000, refreshKey = 0) {
  const [data, setData] = React.useState(null);
  const [error, setError] = React.useState(null);
  const [updatedAt, setUpdatedAt] = React.useState(null);
  const cancelled = React.useRef(false);

  const load = React.useCallback(async () => {
    try {
      const value = await loader();
      if (cancelled.current) return;
      setData(value);
      setError(null);
      setUpdatedAt(new Date());
    } catch (e) {
      if (!cancelled.current) setError(e);
    }
  }, [loader]);

  React.useEffect(() => {
    cancelled.current = false;
    let timer = null;

    const tick = async () => {
      if (document.visibilityState === 'visible') {
        await load();
      }
      if (!cancelled.current) timer = setTimeout(tick, intervalMs);
    };

    tick();
    return () => {
      cancelled.current = true;
      if (timer) clearTimeout(timer);
    };
  }, [load, intervalMs, refreshKey]);

  return { data, error, updatedAt, reload: load };
}
