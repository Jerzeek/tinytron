// Using ES modules for ffmpeg.wasm
import { FFmpeg } from './ffmpeg/index.js';
import { toBlobURL } from './ffmpeg-util/index.js';

document.addEventListener('DOMContentLoaded', () => {
    const transcodeVideoFile = document.getElementById('transcodeVideoFile');
    const transcodeButton = document.getElementById('transcodeButton');
    const transcodeProgress = document.getElementById('transcodeProgress');
    const transcodeStatus = document.getElementById('transcodeStatus');
    const downloadLink = document.getElementById('downloadLink');

    let ffmpeg;
    let detectedFps = 30;

    const loadFFmpeg = async () => {
        transcodeStatus.textContent = 'Loading ffmpeg-core.js';
        ffmpeg = new FFmpeg();
        ffmpeg.on('log', ({ message }) => {
            console.log(message);
            // Look for a pattern like " 30 fps," or " 23.98 fps,"
            const fpsMatch = message.match(/,\s*(\d+(?:\.\d+)?)\s*fps/);
            if (fpsMatch && fpsMatch[1]) {
              detectedFps = parseFloat(fpsMatch[1]);
            }
        });
        ffmpeg.on('progress', ({ progress, time }) => {
            transcodeProgress.value = progress * 100;
        });

        const coreURL = await toBlobURL('https://unpkg.com/@ffmpeg/core@0.12.6/dist/esm/ffmpeg-core.js', 'text/javascript');
        const wasmURL = await toBlobURL('https://unpkg.com/@ffmpeg/core@0.12.6/dist/esm/ffmpeg-core.wasm', 'application/wasm');

        await ffmpeg.load({
            coreURL,
            wasmURL
        });
        transcodeStatus.textContent = 'FFmpeg loaded.';
        transcodeButton.disabled = false;
    };

    transcodeButton.addEventListener('click', async () => {
        if (!transcodeVideoFile.files || transcodeVideoFile.files.length === 0) {
            alert('Please select a video file first.');
            return;
        }

        const file = transcodeVideoFile.files[0];
        transcodeButton.disabled = true;
        transcodeProgress.style.display = 'block';
        transcodeProgress.value = 0;
        downloadLink.style.display = 'none';
        transcodeStatus.textContent = 'Detecting source framerate...';

        await ffmpeg.writeFile('input.mp4', new Uint8Array(await file.arrayBuffer()));
        await ffmpeg.run('-i', 'input.mp4');

        transcodeVideoFile.disabled = true;
        transcodeStatus.textContent += '\nFound ' + detectedFps;
        const targetFps = Math.min(detectedFps, 25);
        transcodeStatus.textContent += '\nTranscoding video at ' + targetFps + ' FPS...';

        const command = [
            '-y',
            '-i',
            'input.mp4',
            '-an',
            '-c:v',
            'mjpeg',
            '-q:v',
            '10',
            '-vf',
            `scale=-1:240:flags=lanczos,crop=288:240:(in_w-288)/2:0,fps=${targetFps}`,
            'out.avi'
        ];

        await ffmpeg.exec(command);

        const data = await ffmpeg.readFile('out.avi');
        const blob = new Blob([data.buffer], { type: 'video/avi' });
        const url = URL.createObjectURL(blob);

        downloadLink.href = url;
        downloadLink.download = 'out.avi';
        downloadLink.style.display = 'block';
        transcodeStatus.textContent += '\nTranscoding complete!';
        transcodeVideoFile.disabled = false;
        transcodeButton.disabled = false;
        transcodeProgress.style.display = 'none';
    });

    loadFFmpeg();
});