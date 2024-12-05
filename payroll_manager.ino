#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#include <MemoryFree.h>;

#define WHITE 7
#define YELLOW 3
#define GREEN 2
#define RED 1
#define PURPLE 5

enum phase { sync,
             main };

struct payRoll {
  int jobGrade;
  bool penStatus;
  float salary;
  String jobRole;
  String employeeID;
};

payRoll accounts[20];

int accountCount = 0;
int index = 0;

char employeeID[9];
char jobGrade[3];
char jobRole[19];
char penStatus[6];
char salary[10];

unsigned long selectPressTime = 0;
bool selectButtonHeld = false;

unsigned long scrollLastTime = 0;
int scrollIndex = -1;

static bool flag = true;

byte customChar[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

byte customChar1[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b01110,
  0b00100
};

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setBacklight(YELLOW);
  lcd.createChar(0, customChar);
  lcd.createChar(1, customChar1);
}

void loop() {
  static int phase = sync;
  static unsigned long myTime = millis();
  uint8_t buttons = lcd.readButtons();

  scroll(index);

  if (buttons & BUTTON_SELECT) {
    if (selectPressTime == 0) {
      selectPressTime = millis();
    } 
    
    else if (millis() - selectPressTime > 1000) {
      lcd.setBacklight(PURPLE);
      lcd.setCursor(0, 0);
      lcd.print(F("F418827          "));
      lcd.setCursor(0, 1);
      lcd.print(freeMemory());
      lcd.print(" BYTES       ");
      selectButtonHeld = true;
      flag = false;
    }
  } else {
    if (selectButtonHeld) {
      if (phase == sync) {
        lcd.clear();
        lcd.setBacklight(YELLOW);
        selectButtonHeld = false;
      } else if (phase == main) {
        if (accountCount > 0) {
          lcd.clear();
          displayAccounts(index);
          flag = true;
          selectButtonHeld = false;
        } else {
          lcd.clear();
          lcd.setBacklight(WHITE);
          selectButtonHeld = false;
        }
      }
      selectPressTime = 0;
    }
  }


  switch (phase) {
    case sync:
      if (millis() - myTime >= 2000) {
        Serial.print(F("R"));
        myTime = millis();
      }

      if (Serial.available() > 0) {
        String input = Serial.readString();
        if (input.indexOf('\n') != -1 || input.indexOf('\r') != -1) {
          Serial.println(F("ERROR: Invalid characters."));
          return;
        } else {

          if (input == "BEGIN") {
            lcd.setBacklight(WHITE);
            Serial.println(F("UDCHARS,FREERAM,SCROLL"));
            phase = main;
          }
        }
      }


      break;

    case main:


      if (Serial.available() > 0) {

        String input = Serial.readString();


        int inputLength = input.length();
        char data[inputLength + 1];
        input.toCharArray(data, inputLength + 1);

        char* command = strtok(data, "-");

        if (strcmp(command, "ADD") == 0) {
          addAccount(input.c_str());
        } else if (strcmp(command, "PST") == 0) {
          setPenStatus(input.c_str());
        } else if (strcmp(command, "GRD") == 0) {
          changeJobGrade(input.c_str());
        } else if (strcmp(command, "SAL") == 0) {
          changeSalary(input.c_str());
        } else if (strcmp(command, "CJT") == 0) {
          changeJobTitle(input.c_str());
        } else if (strcmp(command, "DEL") == 0) {
          deleteAccount(input.c_str());
        } else {
          Serial.println(F("ERROR: Invalid Command"));
          return;
        }

        while (Serial.available() > 0) {
          Serial.read();
        }
      }

      if (buttons & BUTTON_DOWN && index < accountCount - 1) {
        index = index + 1;
        displayAccounts(index);
      }

      if (buttons & BUTTON_UP && index > 0) {
        index = index - 1;
        displayAccounts(index);
      }
      break;
  }
}


void addAccount(char* input) {

  if (dashCount(input) != 3) {
    Serial.println(F("ERROR: Invalid ADD Format"));
    return;
  }
  strtok(input, "-");
  strncpy(employeeID, strtok(NULL, "-"), sizeof(employeeID) - 1);
  employeeID[sizeof(employeeID) - 1] = '\0';

  strncpy(jobGrade, strtok(NULL, "-"), sizeof(jobGrade) - 1);
  jobGrade[sizeof(jobGrade) - 1] = '\0';

  strncpy(jobRole, strtok(NULL, "-"), sizeof(jobRole) - 1);
  jobRole[sizeof(jobRole) - 1] = '\0';

  if (accExsist(employeeID) == true) {
    Serial.println(F("ERROR: Account Already Exsists"));
    return;
  }

  else if (strlen(employeeID) != 7 || isItOnlyDigits(employeeID) == false) {
    Serial.println(F("ERROR: Invalid Employee ID"));
    return;
  }

  else if (strlen(jobGrade) != 1 || verifyJobGrade(jobGrade) == false) {
    Serial.println(F("ERROR: Invalid Job Grade"));
    return;
  }

  else if (strlen(jobRole) < 3 || strlen(jobRole) > 17 || verifyJobRole(jobRole) == false) {
    Serial.println(F("ERROR: Invalid Job Role"));
    return;
  }

  else {
    if (accountCount < 20) {
      accounts[accountCount].employeeID = String(employeeID);
      accounts[accountCount].jobGrade = atoi(jobGrade);
      accounts[accountCount].jobRole = String(jobRole);
      accounts[accountCount].penStatus = true;
      accounts[accountCount].salary = 00000.00;
      accountCount = accountCount + 1;
      Serial.println(F("DONE: Account Added Successfuly!"));
      displayAccounts(index);
    } else {
      Serial.println(F("ERROR: Account Limit Reached"));
    }
  }
}


void setPenStatus(char* input) {

  if (dashCount(input) != 2) {
    Serial.println(F("ERROR: Invalid PST Format"));
    return;
  }

  strtok(input, "-");
  strncpy(employeeID, strtok(NULL, "-"), sizeof(employeeID) - 1);
  employeeID[sizeof(employeeID) - 1] = '\0';

  strncpy(penStatus, strtok(NULL, "-"), sizeof(penStatus) - 1);
  penStatus[sizeof(penStatus) - 1] = '\0';

  int x = findAcc(employeeID);

  if (strlen(employeeID) != 7 || isItOnlyDigits(employeeID) == false) {
    Serial.println(F("ERROR: Invalid Employee ID"));
    return;
  }

  else if (x == -1) {
    Serial.println(F("ERROR: Account Does Not Exsist"));
    return;
  }

  else if (strcmp(penStatus, "PEN") != 0 && strcmp(penStatus, "NPEN") != 0) {
    Serial.println(F("ERROR: Invalid Pension Status"));
    return;
  }

  else if (accounts[x].salary == 00000.00) {
    Serial.println(F("ERROR: Salary Is Still £00000.00"));
    return;
  }

  else {
    if (strcmp(penStatus, "PEN") == 0) {
      bool newPenStatus = true;
      if (newPenStatus == accounts[x].penStatus) {
        Serial.println(F("ERROR: Pension Status Unchanged"));
        return;
      }

      else {
        accounts[x].penStatus = true;
        Serial.println(F("DONE: Pension Status Changed Successfully"));
        displayAccounts(index);
      }
    } else if (strcmp(penStatus, "NPEN") == 0) {
      bool newPenStatus = false;
      if (newPenStatus == accounts[x].penStatus) {
        Serial.println(F("ERROR: Pension Status Unchanged"));
        return;
      }

      else {
        accounts[x].penStatus = false;
        Serial.println(F("DONE: Pension Status Changed Successfully"));
        displayAccounts(index);
      }
    }
  }
}


void changeJobGrade(char* input) {

  if (dashCount(input) != 2) {
    Serial.println(F("ERROR: Invalid GRD Format"));
    return;
  }

  strtok(input, "-");
  strncpy(employeeID, strtok(NULL, "-"), sizeof(employeeID) - 1);
  employeeID[sizeof(employeeID) - 1] = '\0';

  strncpy(jobGrade, strtok(NULL, "-"), sizeof(jobGrade) - 1);
  jobGrade[sizeof(jobGrade) - 1] = '\0';

  int x = findAcc(employeeID);

  if (strlen(employeeID) != 7 || isItOnlyDigits(employeeID) == false) {
    Serial.println(F("ERROR: Invalid Employee ID"));
    return;
  }

  else if (x == -1) {
    Serial.println(F("ERROR: Account Does Not Exsist"));
    return;
  }

  else if (strlen(jobGrade) != 1 || verifyJobGrade(jobGrade) == false) {
    Serial.println(F("ERROR: Invalid Job Grade"));
    return;
  }

  else {
    int newJobGrade = atoi(jobGrade);
    if (accounts[x].jobGrade > newJobGrade) {
      Serial.println(F("ERROR: You Can Not Change To A Lower Job Grade"));
      return;
    }

    else if (accounts[x].jobGrade == newJobGrade) {
      Serial.println(F("ERROR: You Can Not Change To The Same Job Grade"));
      return;
    }

    else {
      accounts[x].jobGrade = newJobGrade;
      Serial.println(F("DONE: Job Grade Changed Succesfully!"));
      displayAccounts(index);
    }
  }
}


void changeSalary(char* input) {

  if (dashCount(input) != 2) {
    Serial.println(F("ERROR: Invalid SAL Format"));
    return;
  }

  strtok(input, "-");
  strncpy(employeeID, strtok(NULL, "-"), sizeof(employeeID) - 1);
  employeeID[sizeof(employeeID) - 1] = '\0';

  strncpy(salary, strtok(NULL, "-"), sizeof(salary) - 1);
  salary[sizeof(salary) - 1] = '\0';

  int x = findAcc(employeeID);

  if (strlen(employeeID) != 7 || isItOnlyDigits(employeeID) == false) {
    Serial.println(F("ERROR: Invalid Employee ID"));
    return;
  }

  else if (x == -1) {
    Serial.println(F("ERROR: Account Does Not Exsist"));
    return;
  }

  else if (verifySalary(salary) == false) {
    Serial.println(F("ERROR: Invalid Salary"));
    return;
  }

  else {
    float newSalary = atof(salary);
    if (newSalary < 00000.00 || newSalary >= 100000.00) {
      Serial.println(F("ERROR: Invalid Salary"));
      return;
    }

    else {
      newSalary = round(newSalary * 100.0) / 100.0;
      accounts[x].salary = newSalary;
      Serial.println(F("DONE: Salary Changed Successfully"));
      displayAccounts(index);
    }
  }
}


void changeJobTitle(char* input) {

  if (dashCount(input) != 2) {
    Serial.println(F("ERROR: Invalid CJT Format"));
    return;
  }

  strtok(input, "-");
  strncpy(employeeID, strtok(NULL, "-"), sizeof(employeeID) - 1);
  employeeID[sizeof(employeeID) - 1] = '\0';

  strncpy(jobRole, strtok(NULL, "-"), sizeof(jobRole) - 1);
  jobRole[sizeof(jobRole) - 1] = '\0';

  int x = findAcc(employeeID);

  if (strlen(employeeID) != 7 || isItOnlyDigits(employeeID) == false) {
    Serial.println(F("ERROR: Invalid Employee ID"));
    return;
  }

  else if (x == -1) {
    Serial.println(F("ERROR: Account Does Not Exsist"));
    return;
  }

  else if (strlen(jobRole) < 3 || strlen(jobRole) > 17 || verifyJobRole(jobRole) == false) {
    Serial.println(F("ERROR: Invalid Job Role"));
    return;
  }

  else if (accounts[x].jobRole == jobRole) {
    Serial.println(F("ERROR: You Can Not Change To The Same Job Title"));
    return;
  }

  else {
    accounts[x].jobRole = String(jobRole);
    Serial.println(F("DONE: Job Title Changed Succesfully!"));
    displayAccounts(index);
  }
}


void deleteAccount(char* input) {

  if (dashCount(input) != 1) {
    Serial.println(F("ERROR: Invalid DEL Format"));
    return;
  }

  strtok(input, "-");
  strncpy(employeeID, strtok(NULL, "-"), sizeof(employeeID) - 1);
  employeeID[sizeof(employeeID) - 1] = '\0';

  int x = findAcc(employeeID);

  if (strlen(employeeID) != 7 || isItOnlyDigits(employeeID) == false) {
    Serial.println(F("ERROR: Invalid Employee ID"));
    return;
  }

  else if (x == -1) {
    Serial.println(F("ERROR: Account Does Not Exsist"));
    return;
  }

  for (int z = x; z < accountCount - 1; z++) {
    accounts[z] = accounts[z + 1];
  }

  accounts[accountCount - 1] = { 0, false, 0.0, "", "" };
  accountCount = accountCount - 1;

  if (index > 0) {
    index = index - 1;
    displayAccounts(index);
    Serial.println(F("DONE: Account Deleted Successfully"));

  }

  else {
    displayAccounts(index);
    Serial.println(F("DONE: Account Deleted Successfully"));
  }

  if (accountCount == 0) {
    Serial.println(F("DONE: Account Deleted Successfully"));
    lcd.clear();
    lcd.setBacklight(WHITE);
  }
}


void displayAccounts(int index) {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" ");
  lcd.setCursor(0, 1);
  lcd.print(" ");

  if (index > 0) {
    lcd.setCursor(0, 0);
    lcd.write(0);
  }


  if (index < accountCount - 1 && accountCount > 1) {
    lcd.setCursor(0, 1);
    lcd.write(1);
  }


  lcd.setCursor(1, 0);
  lcd.print(accounts[index].jobGrade);
  lcd.setCursor(3, 0);
  if (accounts[index].penStatus == true) {
    lcd.print("PEN ");
    lcd.setBacklight(GREEN);
  }

  else if (accounts[index].penStatus == false) {
    lcd.print("NPEN");
    lcd.setBacklight(RED);
  }
  lcd.setCursor(8, 0);
  lcd.print(accounts[index].salary);
  lcd.setCursor(1, 1);
  lcd.print(accounts[index].employeeID);
}


void scroll(int index) {
  lcd.setCursor(9, 1);
  String jobRole = accounts[index].jobRole;

  if (jobRole.length() <= 7 && flag == true) {
    lcd.print(accounts[index].jobRole);
  }

  if (jobRole.length() > 7 && flag == true) {
    if (millis() - scrollLastTime >= 500) {
      scrollIndex += 1;
      if (scrollIndex > jobRole.length() - 7) {
        scrollIndex = 0;
      }
      scrollLastTime = millis();
    }
    lcd.print(jobRole.substring(scrollIndex, scrollIndex + 7));
  }
}


int dashCount(char* message) {
  int dashCount = 0;
  for (int z = 0; z < strlen(message); z++) {
    if (message[z] == '-') {
      dashCount++;
    }
  }
  return dashCount;
}

bool isItOnlyDigits(char* empID) {
  for (int z = 0; z < strlen(empID); z++) {
    if (!isDigit(empID[z])) {
      return false;
    }
  }
  return true;
}

bool accExsist(char* empID) {
  for (int z = 0; z < 20; z++) {
    if (accounts[z].employeeID == empID) {
      return true;
    }
  }
  return false;
}

bool verifyJobGrade(char* jobGrade) {
  for (int z = 0; z < strlen(jobGrade); z++) {
    if (!isDigit(jobGrade[z]) || jobGrade[z] == '0') {
      return false;
    }
  }
  return true;
}

bool verifyJobRole(char* jobRole) {
  for (int z = 0; z < strlen(jobRole); z++) {
    char c = jobRole[z];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.')) {
      return false;
    }
  }
  return true;
}

int findAcc(char* empID) {
  for (int z = -1; z < 20; z++) {
    if (accounts[z].employeeID == empID) {
      return z;
    }
  }
  return -1;
}

bool verifySalary(char* salary) {
  for (int z = 0; z < strlen(salary); z++) {
    char c = salary[z];
    if (!((c >= '0' && c <= '9') || c == '.')) {
      return false;
    }
  }
  return true;
}
