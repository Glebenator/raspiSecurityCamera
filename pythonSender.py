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

def send_email():
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
        try:
            os.remove(image_path)
            print(f"The image {image_path} has been deleted successfully.")
        except FileNotFoundError:
            print("The file was not found, and therefore not deleted.")
        except PermissionError:
            print("Permission denied: unable to delete the file.")
        except Exception as e:
            print(f"An error occurred: {e}")
    except HttpError as error:
        print(f'An error occurred: {error}')
        message = None

    print("Email sent because of person detected alert!")

shutdown_flag = False

def handle_client(client_socket):
    global shutdown_flag
    with client_socket as sock:
        while True:
            try:
                data = sock.recv(1024)
                if not data:
                    break
                
                message = data.decode('utf-8').strip()
                print(f"Received data: {message}")

                if message == '!!':
                    send_email()
                elif message == '@@':
                    print("Shutdown command received! Shutting down server.")
                    shutdown_flag = True
                    break
                   
            except ConnectionResetError:
                print("Connection reset by client.")
                break
            except Exception as e:
                print(e)
                break

def start_server(host='localhost', port=65432):
    global shutdown_flag
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.bind((host, port))
        server.listen()

        print(f'Server listening on {host}:{port}...')

        # Wait for client connections
        while not shutdown_flag:
            client_socket, client_address = server.accept()
            print(f"Connected to {client_address}")

            # Handle client connection in separate thread
            thread = threading.Thread(target=handle_client, args=(client_socket,))
            thread.start()
            thread.join()  # Wait for the thread to finish before possibly checking shutdown_flag

if __name__ == '__main__':
    start_server()