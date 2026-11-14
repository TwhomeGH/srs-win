import React from 'react';
import { Alert, Button, Container } from 'react-bootstrap';

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
      <Container fluid className="py-3">
        <Alert variant="danger">
          <Alert.Heading>Console error</Alert.Heading>
          <div className="small mb-2">{String(this.state.error?.message || this.state.error)}</div>
          <Button size="sm" variant="outline-secondary" onClick={() => this.setState({ error: null })}>
            Retry
          </Button>
        </Alert>
      </Container>
    );
  }
}
