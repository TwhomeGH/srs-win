import React from 'react';

// Bootstrap layout primitives as plain HTML, so the console does not need
// react-bootstrap or its React Aria dependency tree just for a few divs.
export function Container({ fluid = false, className = '', children }) {
  return <div className={`${fluid ? 'container-fluid' : 'container'} ${className}`.trim()}>{children}</div>;
}

export function Row({ className = '', children }) {
  return <div className={`row ${className}`.trim()}>{children}</div>;
}

export function Col({ xs, md, xl, className = '', children }) {
  const classes = ['col'];
  if (xs !== undefined) classes.push(`col-${xs}`);
  if (md !== undefined) classes.push(`col-md-${md}`);
  if (xl !== undefined) classes.push(`col-xl-${xl}`);
  if (className) classes.push(className);
  return <div className={classes.join(' ')}>{children}</div>;
}

export function Card({ title, children }) {
  return (
    <div className="card h-100">
      {title && <div className="card-header">{title}</div>}
      {children}
    </div>
  );
}

export function Table({ responsive = false, children, className = '' }) {
  const table = <table className={`table table-sm table-striped table-bordered table-hover ${className}`.trim()}>{children}</table>;
  return responsive ? <div className="table-responsive">{table}</div> : table;
}

export function Badge({ variant = 'secondary', children, title }) {
  return <span className={`badge text-bg-${variant}`} title={title}>{children}</span>;
}

export function Alert({ variant = 'info', children }) {
  return <div className={`alert alert-${variant}`} role="alert">{children}</div>;
}

export function Button({ variant = 'secondary', size, className = '', children, ...rest }) {
  return (
    <button
      type="button"
      className={`btn btn-${variant} ${size ? `btn-${size}` : ''} ${className}`.trim()}
      {...rest}
    >
      {children}
    </button>
  );
}
