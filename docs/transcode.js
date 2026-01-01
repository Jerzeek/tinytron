import { FFmpeg } from './ffmpeg/index.js';
import { toBlobURL } from './ffmpeg-util/index.js';

document.addEventListener('DOMContentLoaded', () => {
  const transcodeVideoFile = document.getElementById('transcodeVideoFile');
  const transcodeButton = document.getElementById('transcodeButton');
  const cancelButton = document.getElementById('cancelButton');
  const transcodeProgress = document.getElementById('transcodeProgress');
  const transcodeStatus = document.getElementById('transcodeStatus');
  const downloadLink = document.getElementById('downloadLink');

  let ffmpeg = null;
  let detectedFps = 30; // Default fallback

  const log = (message) => {
    transcodeStatus.textContent += message + '\n';
    transcodeStatus.scrollTop = transcodeStatus.scrollHeight;
  };

  const lockUI = (isProcessing = true) => {
      transcodeButton.disabled = isProcessing;
      transcodeButton.style.display = isProcessing ? 'none' : '';
      cancelButton.disabled = !isProcessing;
      cancelButton.style.display = isProcessing ? '' : 'none';
  }

  const loadFFmpeg = async () => {
    log('Loading ffmpeg-core.js...');
    lockUI(false);

    ffmpeg = new FFmpeg();

    ffmpeg.on('log', ({ message }) => {
      console.log(message);
      const fpsMatch = message.match(/,\s*(\d+(?:\.\d+)?)\s*fps/);
      if (fpsMatch && fpsMatch[1]) {
        detectedFps = parseFloat(fpsMatch[1]);
      }
    });

    ffmpeg.on('progress', ({ progress, time }) => {
      // Ensure progress is between 0 and 100
      transcodeProgress.value = Math.min(Math.max(progress * 100, 0), 100);
    });

    const coreURL = await toBlobURL('https://unpkg.com/@ffmpeg/core@0.12.6/dist/esm/ffmpeg-core.js', 'text/javascript');
    const wasmURL = await toBlobURL('https://unpkg.com/@ffmpeg/core@0.12.6/dist/esm/ffmpeg-core.wasm', 'application/wasm');

    await ffmpeg.load({ coreURL, wasmURL });

    log('FFmpeg loaded. Ready.');
    transcodeButton.disabled = false;
  };

  // 2. Transcode Logic
  transcodeButton.addEventListener('click', async () => {
    if (!transcodeVideoFile.files || transcodeVideoFile.files.length === 0) {
      alert('Please select a video file first.');
      return;
    }

    const file = transcodeVideoFile.files[0];

    // UI Updates
    transcodeVideoFile.disabled = true;
    lockUI(true);
    transcodeProgress.value = 0;
    transcodeProgress.style.display = 'block';
    downloadLink.style.display = 'none';

    try {
      log('Detecting source framerate...');
      await ffmpeg.writeFile('input.mp4', new Uint8Array(await file.arrayBuffer()));

      // FIX 1: Use exec with array, and ignore the inevitable error
      try {
        await ffmpeg.exec(['-i', 'input.mp4']);
      } catch (e) {
        // We expect this to fail because there is no output file.
        // We only ran this to trigger the 'log' event to find FPS.
        console.log('Probe finished (expected error ignored)');
      }

      const targetFps = Math.min(detectedFps, 25);
      log(`Found ${detectedFps} FPS.${detectedFps > targetFps ? ` Output will be capped at ${targetFps} FPS to reduce file size` : ''}`);

      const command = [
        '-y',
        '-i', 'input.mp4',
        '-an',
        '-c:v', 'mjpeg',
        '-q:v', '10',
        '-vf', `scale=-1:240:flags=lanczos,crop=320:240:(in_w-320)/2:0,fps=${targetFps}`,
        'out.avi'
      ];

      log(`Running command: ffmpeg ${command.join(' ')}`);
      log(`Transcoding...`);

      await ffmpeg.exec(command);

      // Success handling
      const data = await ffmpeg.readFile('out.avi');
      const blob = new Blob([data.buffer], { type: 'video/avi' });
      const url = URL.createObjectURL(blob);

      downloadLink.href = url;
      downloadLink.download = 'out.avi';
      downloadLink.style.display = 'block';
      log('Transcoding complete!');
    } catch (err) {
      console.error(err);
      log('Error during transcoding (check console).');
    } finally {
      // Cleanup UI
      transcodeVideoFile.disabled = false;
      transcodeProgress.style.display = 'none';
      lockUI(false);
    }
  });

  // 3. Cancel/Interrupt Logic
  cancelButton.addEventListener('click', () => {
    if (ffmpeg) {
      ffmpeg.terminate(); // Kills the worker
      log('Transcoding cancelled. Reloading FFmpeg...');

      // Clean up UI immediately
      transcodeProgress.style.display = 'none';
      transcodeVideoFile.disabled = false;
      // We must reload ffmpeg to use it again
      loadFFmpeg();
    }
  });

  loadFFmpeg();
});