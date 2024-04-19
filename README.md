# 2048
## About

Game Board allows you to play 2048. The game takes place on a board divided into 16 fields, on which initially two fields with the number 2 are available. The aim of the game is to move and combine the fields appearing on the board with the same value to create a field with the number 2048.

## Usage

After turning the board on, a menu appears, from which we can choose one of the 6 available options: 
- Play 2048
- Play 2048-Server
- Play 2048-Client
- Bluetooth
- Show Scores
- Clear Score

We move around the menu by moving the joystick up or down. After selecting the item you are interested in, just press the joystick and you will be taken to the selected section. To exit, press the Reset button.

## Menu

### Play 2048
After selecting the section, the game is launched. A green LED turns on, which goes out gradually. After it turns off, the game can be started. Movements in the game are made by moving the joystick in any direction. The game ends when the score reaches 2048 (win) or when it is impossible to make another move (loss). When the game ends, a buzzer is activated, which plays a sequence of notes that are the beginning of the melody from the Super Mario Bros game. To return to the menu, press Reset.

### Play 2048 - Server
After selecting the section we start a server for playing 2048 together. When another board connects, a message appears asking to accept the connection. After accepting the connection, the game starts. Between moves, information about the results is exchanged. Depending on whether our current score is higher or lower than that of the other tile, a green or red LED is lit, respectively. To return to the menu, press Reset.

### Play 2048 - Client
After selecting the section, the search for a 2048 game server begins. After finding possible devices, a window appears where we can select one of them. After selecting the device we want to connect to, we wait for the server to accept the connection. Once the connection is accepted, the game begins. Between moves, information on the results is exchanged. Depending on whether our current score is higher or lower than the score on the other board, a green or red LED is lit, respectively. To return to the menu, press Reset.

### Bluetooth
After selecting the section, we have the option to change the name and address of the board, and search all available devices that have Bluetooth running. To return to the menu, press Reset. 

### Show Scores
When the section is selected, an array of the eight best scores from the game is displayed. To return to the menu, press the joystick. 

### Clear Scores
When the section is selected, the score data is cleared, resulting in an empty table when the Show Scores section is selected.

## Gameplay
Two fields are drawn, which are filled with the value of 2. After the user selects a direction, the algorithm checks whether the two values from the side of the pressed direction can be combined or moved to the selected side. The fields on the opposite side are then filled with zeros. Each time two fields are combined, the user's score is increased by 1. This is followed by filling in two random fields again and waiting for the user to move. The game continues until all fields are filled with non-zero values or until 2048 points are scored.

## GameBoard specs
The specs of the system can be found [here](https://web.archive.org/web/20061128023501/https://www.embeddedartists.com/products/boards/lpc2104_pro002.php).
