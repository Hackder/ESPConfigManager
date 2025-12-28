import os

print("Running web generation script...")

# Define paths relative to the project root
input_file = os.path.join("lib", "ESPConfigManager", "src", "index.html")
output_file = os.path.join("lib", "ESPConfigManager", "src", "index.h")

if os.path.exists(input_file):
    with open(input_file, "r") as f:
        html_content = f.read()

    with open(output_file, "w") as f:
        f.write('const char* IndexPage = R"=====(\n')
        f.write(html_content)
        f.write('\n)=====";')

    print(f"Successfully generated {output_file}")
else:
    print(f"Error: Could not find {input_file}")
