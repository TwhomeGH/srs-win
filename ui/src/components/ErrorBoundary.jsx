import React from 'react';

export class ErrorBoundary extends React.Component {
  constructor(props) {
    super(props);
    this.state = { error: null };
  }

  static getDerivedStateFromError(error) {
    return { error };
  }

  componentDidCatch(error, info) {
    console.error('SRS console render error', error, info);
  }

  render() {
    if (!this.state.error) return this.props.children;

    return (
      <div className="container-fluid py-3">
        <div className="alert alert-danger" role="alert">
          <h2 className="h5">Console error</h2>
          <div className="small mb-2">{String(this.state.error?.message || this.state.error)}</div>
          <button type="button" className="btn btn-sm btn-outline-secondary" onClick={() => this.setState({ error: null })}>
            Retry
          </button>
        </div>
      </div>
    );
  }
}
