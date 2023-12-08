import socket
import threading
import os
import base64
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

SCOPES = [
    "https://www.googleapis.com/auth/gmail.send"
]
flow = InstalledAppFlow.from_client_secrets_file('credentials.json', SCOPES)
creds = flow.run_local_server(port=0)

service = build('gmail', 'v1', credentials=creds)



# try:
#     message = (service.users().messages().send(userId="me", body=create_message).execute())
#     print(f'Sent message to {msg["to"]} Message Id: {message["id"]}')
# except HttpError as error:
#     print(f'An error occurred: {error}')
#     message = None

def send_email(data):
    # Check the received data and decide whether to send the email
    # Create a multipart message
    msg = MIMEMultipart('related')  # 'related' is used for inline images

    # Create the body with HTML content
    html = """\
    <html>
    <head></head>
    <body>
        <p>Please beware, and check the live feed!</p>
        <img src="cid:image1">  <!-- The 'cid:' is followed by the Content-ID header value -->
    </body>
    </html>
    """
    html_part = MIMEText(html, 'html')
    msg.attach(html_part)

    # Now attach the image to the message.
    image_path = 'detected_person.jpg'  # Replace with the path to your image file
    with open(image_path, 'rb') as img_file:
        img = MIMEImage(img_file.read())
        
        # The Content-ID header value (<image1>) must match the 'cid:' in the HTML part
        img.add_header('Content-ID', '<image1>')
        img.add_header('Content-Disposition', 'inline', filename=os.path.basename(image_path))
        msg.attach(img)

    msg['to'] = 'lilweeb420x69@gmail.com'
    msg['subject'] = 'A person was detected!'

    # Encode the message in base64
    raw_msg = base64.urlsafe_b64encode(msg.as_bytes()).decode()

    create_message = {'raw': raw_msg}
        # Here call the function with all necessary parameters to send the email
        # Call your email sending function...
    try:
        message = (service.users().messages().send(userId="me", body=create_message).execute())
        print(f'Sent message to {msg["to"]} Message Id: {message["id"]}')
    except HttpError as error:
        print(f'An error occurred: {error}')
        message = None

        print("Email sent because of person detected alert!")

# Socket creation function
def start_server(host='localhost', port=65432):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((host, port))
    server.listen()

    print(f'Server listening on {host}:{port}...')

    # Wait for client connections
    while True:
        client_socket, client_address = server.accept()
        print(f"Connected to {client_address}")

        # Handle client connection in separate thread
        thread = threading.Thread(target=handle_client, args=(client_socket,))
        thread.start()

# Function to handle client connection
def handle_client(client_socket):
    with client_socket as sock:
        data = sock.recv(1024)  # Buffer size
        print(f"Received data: {data}")

        # Send email depending on the received data
        if data.decode('utf-8') == '!!':
            send_email(data)

if __name__ == '__main__':
    # Start the server
    start_server()
