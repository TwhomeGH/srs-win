import React from 'react';
import { Container } from './components/Layout';
import { apiGet } from './api';
import { makeT } from './i18n';
import { ErrorBoundary } from './components/ErrorBoundary';
import Overview from './pages/Overview';
import Vhosts from './pages/Vhosts';
import Streams from './pages/Streams';
import Clients from './pages/Clients';
import Configs from './pages/Configs';
import PushDiag from './pages/PushDiag';
import Footer from './components/Footer';

const LANG_KEY = 'srs.console.lang';

const TABS = [
  { key: 'overview', label: 'overview', Page: Overview },
  { key: 'vhosts', label: 'vhosts', Page: Vhosts },
  { key: 'streams', label: 'streams', Page: Streams },
  { key: 'clients', label: 'clients', Page: Clients },
  { key: 'configs', label: 'configs', Page: Configs },
  { key: 'pushdiag', label: 'pushdiag', Page: PushDiag },
];

export default function App() {
  const [lang, setLang] = React.useState(() => localStorage.getItem(LANG_KEY) || 'zh');
  const [tab, setTab] = React.useState('overview');
  const [version, setVersion] = React.useState(null);
  const [offline, setOffline] = React.useState(false);
  const t = React.useMemo(() => makeT(lang), [lang]);

  React.useEffect(() => {
    const reachable = () => setOffline(false);
    const unreachable = () => setOffline(true);
    window.addEventListener('srs:api-reachable', reachable);
    window.addEventListener('srs:api-unreachable', unreachable);
    return () => {
      window.removeEventListener('srs:api-reachable', reachable);
      window.removeEventListener('srs:api-unreachable', unreachable);
    };
  }, []);

  React.useEffect(() => {
    localStorage.setItem(LANG_KEY, lang);
  }, [lang]);

  React.useEffect(() => {
    let cancelled = false;
    apiGet('/versions')
      .then((body) => { if (!cancelled) { setVersion(body.data?.version); setOffline(false); } })
      .catch(() => { if (!cancelled) setOffline(true); });
    return () => { cancelled = true; };
  }, []);

  const active = TABS.find((item) => item.key === tab) || TABS[0];
  const Page = active.Page;

  return (
    <div className="min-vh-100 bg-body-tertiary">
      <nav className="navbar navbar-dark bg-dark px-3">
        <span className="navbar-brand mb-0 h1">SRS Console</span>
        {version && <span className="badge text-bg-secondary me-2">{version}</span>}
        <span className={`badge me-auto ${offline ? 'text-bg-danger' : 'text-bg-success'}`}>
          {offline ? t('apiOffline') : 'API OK'}
        </span>
        <button
          type="button"
          className="btn btn-sm btn-outline-light"
          onClick={() => setLang(lang === 'zh' ? 'en' : 'zh')}
        >
          {lang === 'zh' ? 'EN' : '中文'}
        </button>
      </nav>

      <Container fluid className="py-3">
        <ErrorBoundary>
          <ul className="nav nav-tabs mb-3">
            {TABS.map((item) => (
              <li className="nav-item" key={item.key}>
                <button
                  type="button"
                  className={`nav-link ${item.key === tab ? 'active' : ''}`}
                  onClick={() => setTab(item.key)}
                >
                  {t(item.label)}
                </button>
              </li>
            ))}
          </ul>
          <Page t={t} />
        </ErrorBoundary>
        <Footer />
      </Container>
    </div>
  );
}
