import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

const apiTarget = process.env.SRS_API || 'http://127.0.0.1:11985';

export default defineConfig({
  plugins: [react()],
  base: '/console/',
  build: {
    outDir: 'build',
    emptyOutDir: true,
  },
  server: {
    port: 5173,
    strictPort: true,
    proxy: {
      '/api': { target: apiTarget, changeOrigin: true },
      '/players': { target: apiTarget, changeOrigin: true },
    },
  },
});
