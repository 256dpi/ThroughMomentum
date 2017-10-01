import peasy.*;
import mqtt.*;

PeasyCam cam;
MQTTClient client;

int[] intensity = new int[48];

void setup() {
  size(1920, 1080, P3D);
  frameRate(60);
  
  perspective(radians(90), 4/3, 100, 20000);
  cam = new PeasyCam(this, 100);
  cam.setMinimumDistance(1000);
  cam.setMaximumDistance(10000);
  
  client = new MQTTClient(this);
  client.connect("mqtt://96c342e4:1724bcdee75a6f0b@broker.shiftr.io", "display");
  client.subscribe("activate");
}

void draw() {
  background(0);
  
  for(int k=0; k<48; k++) {
    if(intensity[k] > 0) {
      intensity[k]/=1.1;
    }
  }
  
  translate(-3 * 600 + 300, -4 * 600 + 300);
  
  for(int i=0; i<6; i++) {
   for(int j=0; j<8; j++) {
     int k = i * 8 + j;
     
     pushMatrix();
     fill(30 + intensity[k]);
     translate(i * 600, j * 600, 0);
     box(100, 100, 2000);
     popMatrix(); 
   }
  }
}

void keyPressed() {
  int k = int(random(0, 47));
  intensity[k] = 255;
}

void messageReceived(String topic, byte[] payload) {
  println("new message: " + topic + " - " + new String(payload));
  
  if(topic.equals("activate")) {
    int k = Integer.parseInt(new String(payload));
    intensity[k] = 255;
  }
}