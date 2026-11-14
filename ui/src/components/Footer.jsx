import React from 'react';
import { Container } from 'react-bootstrap';

export default function Footer() {
  return (
    <Container fluid className="text-secondary small py-3">
      SRS-Win web console · API <code>/api/v1</code>
    </Container>
  );
}
