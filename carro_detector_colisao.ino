
#include <AFMotor.h>  
#include <Servo.h>
#include <NewPing.h>


#define VELOCIDADE_MAXIMA 250
#define DISTANCIA_MAXIMA 200 // 2 METROS

// Pinos dos sonares
#define TRIG_PIN_SONAR_E A0 
#define ECHO_PIN_SONAR_E A1 
#define TRIG_PIN_SONAR_D A2 
#define ECHO_PIN_SONAR_D A3 

#define PINO_LED_D A4
#define PINO_LED_E A5
// Criando os objetos do sonares ultrassonicos
NewPing sonarEsquerdo(TRIG_PIN_SONAR_E, ECHO_PIN_SONAR_E, DISTANCIA_MAXIMA); 
NewPing sonarDireito(TRIG_PIN_SONAR_D, ECHO_PIN_SONAR_D, DISTANCIA_MAXIMA);

// Criando os objetos servos que girarão os nossos sensores de detecção
Servo servoEsquerdo;
Servo servoDireito;

// Criando os objetos motores que girarão as rodas do nosso carro
AF_DCMotor motorEsquerdo(1, MOTOR12_1KHZ); 
AF_DCMotor motorDireito( 2, MOTOR12_1KHZ);

// Esses angulos são os angulos em que o sonares ficam "olhando" para frente
const int servoEsquerdoAnguloInicio = 95;
const int servoDireitoAnguloInicio = 115;

const int ESQUERDA = 80;
const int DIREITA = -80;
const int DISTANCIA_MINIMA = 20;

enum LedPin {
  R_LED = A4,   // D7
  L_LED = A5, // D6
};


bool estaMovendoFrente = false;
bool estaVoltando = false;
struct Distancias {
  float direita;
  float frenteD; // distancia da frente do carrinho captada pelo sonar direito
  float frenteE; // distancia da frente do carrinho captada pelo sonar direito
  float esquerda;
};

// Esse variaveis guarda as distancias captadas por cada sonar
Distancias distancia = {0,0,0};


int lerSonarNoAngulo(Servo servo, NewPing sonar, int angulo, int anguloInicial = -1);
void blinkLed(const LedPin led, const int times = 5, const int durationOn = 100, const int durationOff = 100, const bool stayOn = false);

void setup() {
  Serial.begin(9600);
  pinMode(PINO_LED_D, OUTPUT);
  pinMode(PINO_LED_E, OUTPUT);
  servoEsquerdo.attach(10); // Anexa o objeto de Servo ao pino digital 9
  servoDireito.attach(9);   // Anexa o objeto de Servo ao pino digital 10

  servoDireito.write(servoDireitoAnguloInicio);
  servoEsquerdo.write(servoEsquerdoAnguloInicio);

  distancia.frenteD  = lerSonarNoAngulo(servoDireito, sonarDireito, 0, servoDireitoAnguloInicio);
  distancia.frenteE  = lerSonarNoAngulo(servoEsquerdo, sonarEsquerdo, 0, servoEsquerdoAnguloInicio);
  distancia.esquerda = lerSonarNoAngulo(servoDireito, sonarDireito, DIREITA, servoDireitoAnguloInicio);
  distancia.direita  = lerSonarNoAngulo(servoEsquerdo, sonarEsquerdo, ESQUERDA, servoEsquerdoAnguloInicio);


  delay(500);
}


void loop() {

  distancia.frenteE =  lerSonarNoAngulo(servoEsquerdo, sonarEsquerdo, 0, servoEsquerdoAnguloInicio) ;

  if (distancia.frenteE < DISTANCIA_MINIMA ) {
    pensar();
  }

  delay(200);  

  distancia.frenteD =  lerSonarNoAngulo(servoDireito, sonarDireito, 0, servoDireitoAnguloInicio) ;
  
  if (distancia.frenteD < DISTANCIA_MINIMA ) {
    pensar();
  } 

  if ( distancia.frenteD > DISTANCIA_MINIMA && distancia.frenteE > DISTANCIA_MINIMA) {
    if (!estaVoltando) {
      moverFrente(VELOCIDADE_MAXIMA);
    }
  } 

  delay(200);
}


int lerSonarNoAngulo(Servo servo, NewPing sonar, int angulo, int anguloInicial = -1) {

  if (anguloInicial == -1) {
    anguloInicial = servo.read();
  }

  int novaPosicao = anguloInicial + angulo; // Calcula a nova posição do servo
  
  // Verifica se a nova posição está dentro dos limites permitidos (0 a 180 graus)
  if (novaPosicao < 0) {
    novaPosicao = 0;
  }
  else if (novaPosicao > 180) {
    novaPosicao = 180;
  }
  
  servo.write(novaPosicao); // Move o servo para a nova posição
  delay(200); // Aguarda o servo atingir a nova posição

  return lerSonar(sonar);
}


void pensar() {
  if (distancia.frenteE < DISTANCIA_MINIMA || distancia.frenteD < DISTANCIA_MINIMA) {
    moverVoltar(230, 600);
    estaVoltando = true;

    bool virou = tentarFazerCurva(60, 900);
    delay(100);

    if (!virou) {
      tentarFazerCurva(90, 1300);
    }
      
    
  }
}


bool tentarFazerCurva(int angulo, int duracaoCurva) {
    int distanciaD = lerSonarNoAngulo(servoDireito, sonarDireito, angulo * -1, servoDireitoAnguloInicio);
    int distanciaE = lerSonarNoAngulo(servoEsquerdo, sonarEsquerdo, angulo, servoEsquerdoAnguloInicio);

    Serial.println();
    Serial.println(distanciaD);
    Serial.println(distanciaE);
    Serial.println();

    if ( distanciaD > DISTANCIA_MINIMA && distanciaD > distanciaE ) {
      virar('D', duracaoCurva);
      return true;      
    }
    if ( distanciaE > DISTANCIA_MINIMA && distanciaE > distanciaD ) {
      virar('E', duracaoCurva);
      return true;
    }

    // se as distancias forem iguais, escolha um caminho aleatorio para ir
    if ( distanciaE == distanciaD &&  distanciaD > DISTANCIA_MINIMA) {
      int aleatorio = random(0, 2); // gera um número aleatório entre 0 e 1
      if (aleatorio == 0) virar('D', duracaoCurva);
      else virar('E', duracaoCurva);
      return true;
    }
    return false;
}


int lerSonar(NewPing sonar) {
  int cm = sonar.ping_cm();
  if(cm==0)
  {
    cm = DISTANCIA_MINIMA + 1;
  }
  return cm;
}


void virar(char direcao, int duracao) {
  // Define a velocidade das rodas dianteiras
  estaVoltando = false;
  int velocidade = 250;
  // Define a direção das rodas dianteiras
  int direcao_motorDireito, direcao_motorEsquerdo, speed_motorDireito, speed_motorEsquerdo;

  if (direcao == 'E') { // vira para direita
    direcao_motorDireito = FORWARD;
    direcao_motorEsquerdo = BACKWARD;
    speed_motorDireito = velocidade;
    speed_motorEsquerdo = velocidade - 25;

  } else { // vira para esquerda
    direcao_motorDireito = BACKWARD;
    direcao_motorEsquerdo = FORWARD;
    speed_motorDireito = velocidade - 25;
    speed_motorEsquerdo = velocidade;
  }

  // Acelera gradualmente para evitar sobrecarga das baterias
  acelerar(velocidade);
  

  motorDireito.setSpeed(speed_motorDireito);
  motorEsquerdo.setSpeed(speed_motorEsquerdo);
  // Movimenta as rodas dianteiras
  motorDireito.run(direcao_motorDireito);      
  motorEsquerdo.run(direcao_motorEsquerdo);


  if (direcao == 'E') blinkLed(L_LED, 5, duracao / 10, duracao / 10);
  else {blinkLed(R_LED, 5, duracao / 10, duracao / 10);}
  // Espera um tempo para que o carro vire na direção desejada
  // delay(duracao);

  pararMotores();  

  // Para as rodas dianteiras 
  desacelerar(velocidade); 
}


void acelerar(int velocidade) {
  for (int speedSet = 0; speedSet <= velocidade; speedSet += 10) {
    motorDireito.setSpeed(speedSet);
    motorEsquerdo.setSpeed(speedSet);
    delay(10);
  }
}


void desacelerar(int velocidade) {
  for (int speedSet = velocidade; speedSet >= 0; speedSet -= 10) {
    motorDireito.setSpeed(speedSet);
    motorEsquerdo.setSpeed(speedSet);
    delay(10);
  }  
}


void moverFrente(int velocidade) {

  if (!estaMovendoFrente) {
    motorDireito.run(FORWARD);      
    motorEsquerdo.run(FORWARD);
    estaMovendoFrente = true;

    acelerar(velocidade);
  }

}


void moverVoltar(int velocidade, int duracao) {

  estaMovendoFrente = false;

  motorDireito.run(BACKWARD);      
  motorEsquerdo.run(BACKWARD); 

  acelerar(velocidade);

  delay(duracao);
  pararMotores();
}


void pararMotores(){

  estaMovendoFrente = false;
  motorEsquerdo.run(RELEASE); 
  motorDireito.run(RELEASE);

}

void blinkLed(const LedPin led, const int times = 5, const int durationOn = 100, const int durationOff = 100, const bool stayOn = false) {
  
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH);
    delay(durationOn);
    digitalWrite(led, LOW);
    delay(durationOff);
  }

  if (stayOn) {
    digitalWrite(led, HIGH);
  }

}