import base64

def file_to_base64(file_path):
    try:
        with open(file_path, "rb") as file:
            # Read the file and encode it to Base64
            base64_encoded = base64.b64encode(file.read()).decode('utf-8')
        return base64_encoded
    except FileNotFoundError:
        return "File not found. Please check the file path."
    except Exception as e:
        return f"An error occurred: {e}"

# Example usage
file_path = "rudderkick.svg"  # Replace with your file path
base64_string = file_to_base64(file_path)
print(base64_string)

