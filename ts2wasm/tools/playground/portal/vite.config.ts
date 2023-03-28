import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import monacoEditorPlugin from 'vite-plugin-monaco-editor';

// https://vitejs.dev/config/
export default defineConfig(({ command, mode }) => {
    const config = {
        plugins: [react(), monacoEditorPlugin.default({})],
        server: {
            host: '0.0.0.0',
        },
        // octokit.js not work with vite, this is a workaround from:
        // https://stackoverflow.com/questions/73095592/octokit-js-not-working-with-vite-module-externalized-and-cannot-be-accessed-in
        resolve: {
            alias: {
                'node-fetch': 'isomorphic-fetch',
            },
        },
        build: {
            sourcemap: 'inline',
        },
    };

    if (command !== 'build') {
        config['define'] = {
            global: {},
        };
    }

    return config;
});
