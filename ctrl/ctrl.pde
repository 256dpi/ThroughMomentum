// https://shiftr.io/256dpi/vas17

import mqtt.*;

MQTTClient client;

boolean led = false;
int dist = 0;

void setup() {
  size(800, 600);
  frameRate(60);

  client = new MQTTClient(this);
  client.connect("mqtt://96c342e4:1724bcdee75a6f0b@broker.shiftr.io", "processing");
  client.subscribe("dist");
}

void draw() {
  background(255);
  stroke(0);
  fill(0);
  textSize(50);

  text("Dist", 50, 75);
  text(str(dist), 200, 75);
}

void keyPressed() {
  switch (keyCode) {
    // enter
    case 10: {
      led = !led;
      
      if(led) {
        client.publish("/red", "1023");
        client.publish("/green", "1023");
        client.publish("/blue", "256");
      } else {
        client.publish("/red", "0");
        client.publish("/green", "0");
        client.publish("/blue", "0");
      }
      
      break;
    }
    
    // left
    case 39:
      client.publish("/stepper/power", "on");
      client.publish("/stepper/frequency", "900");
      client.publish("/stepper/drive", "cw");
      break;

    // right
    case 37:
      client.publish("/stepper/power", "on");
      client.publish("/stepper/frequency", "900");
      client.publish("/stepper/drive", "ccw");
      break;

    // space
    case 32:
      client.publish("/stepper/power", "off");
      break;

    default:
      println(keyCode);
      break;
  }
}

void messageReceived(String topic, byte[] payload) {
  if(topic.equals("dist")) {
   dist = int(new String(payload));
  } else {
    println("new message: " + topic + " - " + new String(payload));
  }
}