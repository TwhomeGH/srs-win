import React from 'react';
import { Button, Container, Tab, Tabs } from 'react-bootstrap';
import { apiGet } from './api';
import { makeT } from './i18n';
import { ErrorBoundary } from './components/ErrorBoundary';
import Overview from './pages/Overview';
import Vhosts from './pages/Vhosts';
import Streams from './pages/Streams';
import Clients from './pages/Clients';
import Configs from './pages/Configs';
import Footer from './components/Footer';

const LANG_KEY = 'srs.console.lang';

export default function App() {
  const [lang, setLang] = React.useState(() => localStorage.getItem(LANG_KEY) || 'zh');
  const [tab, setTab] = React.useState('overview');
  const [version, setVersion] = React.useState(null);
  const [apiError, setApiError] = React.useState(null);
  const t = React.useMemo(() => makeT(lang), [lang]);

  React.useEffect(() => {
    localStorage.setItem(LANG_KEY, lang);
  }, [lang]);

  React.useEffect(() => {
    let cancelled = false;
    apiGet('/versions')
      .then((body) => { if (!cancelled) { setVersion(body.data?.version); setApiError(null); } })
      .catch((e) => { if (!cancelled) setApiError(e); });
    return () => { cancelled = true; };
  }, []);

  return (
    <div className="min-vh-100 bg-body-tertiary">
      <nav className="navbar navbar-dark bg-dark px-3">
        <span className="navbar-brand mb-0 h1">SRS Console</span>
        {version && <span className="badge text-bg-secondary me-2">{version}</span>}
        <span className={`badge me-auto ${apiError ? 'text-bg-danger' : 'text-bg-success'}`}>
          {apiError ? t('apiError') : 'API OK'}
        </span>
        <Button size="sm" variant="outline-light" onClick={() => setLang(lang === 'zh' ? 'en' : 'zh')}>
          {lang === 'zh' ? 'EN' : '中文'}
        </Button>
      </nav>

      <Container fluid className="py-3">
        <ErrorBoundary>
          <Tabs activeKey={tab} onSelect={(k) => setTab(k || 'overview')} className="mb-3">
            <Tab eventKey="overview" title={t('overview')}><Overview t={t} /></Tab>
            <Tab eventKey="vhosts" title={t('vhosts')}><Vhosts t={t} /></Tab>
            <Tab eventKey="streams" title={t('streams')}><Streams t={t} /></Tab>
            <Tab eventKey="clients" title={t('clients')}><Clients t={t} /></Tab>
            <Tab eventKey="configs" title={t('configs')}><Configs t={t} /></Tab>
          </Tabs>
        </ErrorBoundary>
        <Footer />
      </Container>
    </div>
  );
}
