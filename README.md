# RC_Projeto_23-24
 
## Description

This project is a server-client application written in C. It allows users to interact with assets and auctions, with functionalities such as bidding, listing auctions, and showing assets.

## Directory Structure

- `DB/`: Contains the Auction Server (AS) files.
- `src/`: Contains the source code files.
  - `server.c`: This is the main server program. It handles incoming connections and processes requests.
  - `client.c`: This is the main client program. It sends requests to the server and processes responses.
  - `common.c`: This file contains functions and definitions that are common to both the server and client.

## How to Run

1. Compile the project using the provided Makefile: `make`
2. Run the server: `./server`
    - ./AS [-p ASport] [-v]
    - ASport: Well-known port where the AS server accepts requests, both in UDP and TCP. If omitted, it assumes the value 58000+GN, where GN is the number of the group.
3. Run the client: `./user`
    - ./user [-n ASIP] [-p ASport]
    - ASIP: IP address of the machine where the auction server (AS) runs. If this argument is omitted, the AS should be running on the same machine.
    - ASport: Well-known port (TCP and UDP) where the AS accepts requests.If omitted, it assumes the value 58000+GN, where GN is the group number.

## User Commands
User ID → UID - 6-digit IST student number
Password → composed of 8 alphanumeric (only letters and digits) characters

- `login <username> <password>`: This command allows a user to log into the system. Replace `<username>` and `<password>` with your actual username and password.
- `logout`: This command allows a user to log out of the system.
- `open <name> <asset_fname> <start_value> <timeactive>`: This command allows a user to open a new auction. Replace `<name>`, `<asset_fname>`, `<start_value>`, and `<timeactive>` with the name of the auction, the filename of the asset, the starting value of the auction, and the active time of the auction, respectively.
- `close <auction_id>`: This command allows a user to close an auction. Replace `<auction_id>` with the ID of the auction you want to close.
- `myauctions` or `ma`: This command lists all the auctions created by the logged-in user.
- `mybids` or `mb`: This command lists all the bids placed by the logged-in user.
- `list` or `l`: This command lists all the available auctions.
- `show_asset <auction_id>` or `sa <auction_id>`: This command shows the asset of a specific auction. Replace `<auction_id>` with the ID of the auction whose asset you want to view.
- `bid <auction_id> <amount>` or `b <auction_id> <amount>`: This command allows a user to place a bid on an auction. Replace `<auction_id>` with the ID of the auction you want to bid on, and `<amount>` with the amount you want to bid.
- `show_record <auction_id>` or `sr <auction_id>`: This command shows the record of a specific auction. Replace `<auction_id>` with the ID of the auction whose record you want to view.
- `unregister`: This command allows a user to unregister from the system. If the user is not logged in, a message will be displayed asking the user to log in first.
- `exit`: This command allows a user to exit the system. If the user is logged in, a message will be displayed asking the user to log out first.