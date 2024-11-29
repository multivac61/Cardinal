import json
import os
from collections import defaultdict


def find_duplicate_slugs(start_path):
    # Dictionary to store slug -> [(plugin_name, author)] mappings
    slug_map = defaultdict(list)

    # Walk through directory tree
    for root, dirs, files in os.walk(start_path):
        if "plugin.json" in files:
            json_path = os.path.join(root, "plugin.json")

            try:
                with open(json_path, "r") as f:
                    data = json.load(f)

                    plugin_name = data.get("name", "Unknown")
                    author = data.get("author", "Unknown")
                    modules = data.get("modules", [])

                    # Store each module slug with its plugin info
                    for module in modules:
                        if "slug" in module:
                            slug_map[module["slug"]].append((plugin_name, author))

            except json.JSONDecodeError:
                print(f"Error decoding JSON from {json_path}")
            except Exception as e:
                print(f"Error processing {json_path}: {str(e)}")

    # Print only duplicates
    print("\nDuplicate slugs found:")
    for slug, plugins in slug_map.items():
        if len(plugins) > 1:
            print(f"\nSlug: {slug}")
            for plugin_name, author in plugins:
                print(f"- Found in {plugin_name} by {author}")


if __name__ == "__main__":
    # Start from current directory
    start_dir = "."
    find_duplicate_slugs(start_dir)
