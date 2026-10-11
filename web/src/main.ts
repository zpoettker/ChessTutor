import createChessModule from './wasm/chess.js';

const status = document.querySelector<HTMLParagraphElement>('#status')!;

try {
  const chess = await createChessModule();
  status.textContent = `C++ core version: ${chess.version()}`;
} catch (err) {
  status.textContent = `Failed to load WebAssembly: ${err}`;
  console.error(err);
}
