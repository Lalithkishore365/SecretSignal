from flask import Flask, request, render_template
import serial
import time

app = Flask(__name__)

user_message = ""      # Store user message
encrypted_data = ""
decrypted_data = ""
show_encrypted = False
transmission_mode = ""

@app.route('/')
def index():
    return render_template("index.html", encrypted_data=encrypted_data, show_encrypted=show_encrypted)

@app.route('/send', methods=['POST'])
def send():
    global user_message
    user_message = request.form['user_input']  # Save user message only
    return render_template("transmit_option.html", message=user_message)

@app.route('/transmit', methods=['POST'])
def transmit():
    global encrypted_data, decrypted_data, show_encrypted, user_message, transmission_mode
    mode = request.form['mode']
    transmission_mode = mode

    if mode == 'led':
        control_flag = '1'
    elif mode == 'buzzer':
        control_flag = '3'  # New flag for buzzer mode
    elif mode == 'decrypt':
        control_flag = '2'
    else:  # Just display
        control_flag = '0'

    final_message = f"{user_message}#{control_flag}"

    with serial.Serial('COM10', 9600, timeout=1) as ser:
        ser.write(final_message.encode('utf-8'))
        ser.write(b'\r')
        time.sleep(0.5)

        formatted_data = ""
        while ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                formatted_data += f"{line}<br>"
            except UnicodeDecodeError as e:
                formatted_data += f"Error decoding line: {e}<br>"

        encrypted_data = formatted_data
        
        # For decrypt mode, we'll assume the microcontroller sends the decrypted data
        if mode == 'decrypt':
            decrypted_data = user_message  # In a real implementation, this would be from the microcontroller

    show_encrypted = mode != 'decrypt'  # Don't show encrypted by default in decrypt mode
    
    return render_template(
        "transmit_result.html", 
        encrypted_data=encrypted_data, 
        decrypted_data=decrypted_data,
        mode=mode,
        show_encrypted=show_encrypted
    )

@app.route('/toggle_encryption', methods=['POST'])
def toggle_encryption():
    global show_encrypted
    show_encrypted = not show_encrypted
    return render_template(
        "transmit_result.html", 
        encrypted_data=encrypted_data, 
        decrypted_data=decrypted_data,
        mode=transmission_mode,
        show_encrypted=show_encrypted
    )

if __name__ == '__main__':
    app.run(debug=True)
