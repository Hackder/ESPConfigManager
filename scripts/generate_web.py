import os

import minify_html

print("Running web generation script...")

# Define paths relative to the project root
input_file = os.path.join("lib", "ESPConfigManager", "src", "index.html")
output_file = os.path.join("lib", "ESPConfigManager", "src", "index.h")

if os.path.exists(input_file):
    with open(input_file, "r") as f:
        html_content = f.read()

    minified = minify_html.minify(
        html_content,
        minify_js=True,
        minify_css=True,
        keep_comments=False,
        remove_processing_instructions=True,
    )

    with open(output_file, "w") as f:
        f.write('const char* IndexPage = R"=====(\n')
        f.write(minified)
        f.write('\n)=====";')

    print(f"Successfully generated {output_file}")
else:
    print(f"Error: Could not find {input_file}")
