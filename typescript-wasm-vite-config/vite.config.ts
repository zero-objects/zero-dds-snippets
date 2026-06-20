// Verbatim snippet from website/bindings/typescript-wasm/index.html (Vite)
export default {
    optimizeDeps: { exclude: ['@zerodds/wasm'] },
    server: { fs: { allow: ['..'] } },
};
