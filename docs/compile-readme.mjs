// docs/compile-readme.js
import { marked } from 'marked';
import fs from 'fs';
import path from 'path';

// Define paths
const inputPath = path.resolve(process.cwd(), '..', 'README.md'); // Points to root README.md
// Create dist folder if necessary
if (!fs.existsSync(path.resolve(process.cwd(), 'dist'))) {
    fs.mkdirSync(path.resolve(process.cwd(), 'dist'));
}
const outputPath = path.resolve(process.cwd(), 'dist', 'index.html');
const templatePath = path.resolve(process.cwd(), 'readme-template.html');

// Add anchor links to headings, like on GitHub
const renderer = {
  heading(text, depth) {
    const escapedText = text.toLowerCase().replace(/[^\w]+/g, '-');

    return `
            <h${depth}>
              <a name="${escapedText}" class="anchor" href="#${escapedText}">
                <span class="header-link"></span>
              </a>
              ${text}
            </h${depth}>`;
  }
};
marked.use({ renderer });

try {
    // 1. Read the Markdown content
    const markdown = fs.readFileSync(inputPath, 'utf8');
    
    // 2. Convert to HTML
    const htmlContent = marked(markdown);

    // 3. Read the HTML template (for styling and structure)
    let template = fs.readFileSync(templatePath, 'utf8');

    // 4. Insert the generated HTML into the template
    const finalHtml = template.replace('{content}', htmlContent);

    // 5. Write the final HTML file to the dist folder
    fs.writeFileSync(outputPath, finalHtml);

    console.log(`Successfully compiled ${inputPath} to ${outputPath}`);

} catch (error) {
    console.error('Error during Markdown compilation:', error);
    process.exit(1);
}