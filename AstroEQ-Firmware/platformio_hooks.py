Import("env")
import os
import re
import shutil

print("Current CLI targets", COMMAND_LINE_TARGETS)
print("Current Build targets", list(map(str, BUILD_TARGETS)))

def copy_and_replace(source_path, destination_path):
    if os.path.exists(destination_path):
        os.remove(destination_path)
    shutil.copy2(source_path, destination_path)
    
def current_version() -> str:
    vernumFile = open("VerNum.txt", 'r')
    vernum = vernumFile.read()
    vernumFile.close()
    return vernum

def after_build(source, target, env):
    source = next(map(str, source))
    board = re.findall(r'build[\\/](\w+)[\\/]', source)
    if board:
        board = board[0]
    else:
        board = "???"
    vernum = current_version()
    print(f"Build Finished for {source} => {board}")

    if board == "ATmega162":
        boardNames = ["AstroEQV4-DIYBoard(includingKits)", "AstroEQV4-EQ5Board"]
        boardTitles = ["AstroEQ V4-DIY Board (including DIY)", "AstroEQ V4-EQ5 Board"]
        boardMCU = "atmega162"
        boardBootloader = "arduino"
        boardBaudRate = "57600"
    elif board == "Mega2560":
        boardNames = ["AstroEQArduinoMega1280"]
        boardTitles = ["AstroEQ Arduino Mega 1280"]
        boardMCU = "atmega1280"
        boardBootloader = "arduino"
        boardBaudRate = "57600"
    elif board == "Mega2560":
        boardNames = ["AstroEQArduinoMega2560"]
        boardTitles = ["AstroEQ Arduino Mega 2560"]
        boardMCU = "atmega2560"
        boardBootloader = "wiring"
        boardBaudRate = "115200"
    elif board == "Ramps14":
        boardNames = ["AstroEQRamps14Mega2560"]
        boardTitles = ["AstroEQ Ramps 1.4 w/ Mega 2560"]
        boardMCU = "atmega2560"
        boardBootloader = "wiring"
        boardBaudRate = "115200"
    else:
        print("Unknown board. Skipping post-build")
        return

    outputDir = "../AstroEQ-ConfigUtility/hex/"
    versionFile = outputDir + "versions.txt"
    print(f"Updating version file {versionFile}:\n -> {boardNames},{vernum}, {boardTitles},{boardMCU},{boardBootloader},{boardBaudRate}")

    # Copy new file across
    for boardName in boardNames:
        copy_and_replace(source, f"{outputDir}{boardName}.hex")

    # Load current version file content
    with open(versionFile, 'r') as file:
        lines = [line.rstrip() for line in file]
        file.close()

    # Write to file, updating any lines required
    versionFileHandle = open(versionFile, 'w')
    for line in lines:
        for boardName, boardTitle in zip(boardNames, boardTitles):
            reMatchStr = f"{re.escape(boardName)}.*"
            reReplaceStr = f"{boardName}\t{vernum}\t{boardTitle}\t{boardMCU}\t{boardBootloader}\t{boardBaudRate}"
            line = re.sub(reMatchStr, reReplaceStr, line)
        versionFileHandle.write(f"{line}\n")
    
    

env.AddPostAction("buildprog", after_build)


print("Updating Auto-Gen Version Number File")

vernum = current_version()

print(f"Parsed version: {vernum}")

vernumStrFile = open("VerNumStr.txt", "w")
vernumStrFile.write(f"\"{vernum}\"\n")
vernumStrFile.write("//AUTO GENERATED - DO NOT MODIFY\n")
vernumStrFile.close()

print("Complete. Building files")
