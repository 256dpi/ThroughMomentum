//
//  MainViewController.swift
//  Controller
//
//  Created by Joël Gähwiler on 08.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit
import CocoaMQTT

let lightsPerRow = 8
let lightsPerColumn = 6
let padding: Double = 150
let paddingTop: Double = 200

let offColor = UIColor(white: 0.1, alpha: 1)
let onColor = UIColor(white: 1, alpha: 1)

class MainViewController: UIViewController, CocoaMQTTDelegate {
    var circleViews: [CircleView]?
    var client: CocoaMQTT?
    var connected = false
    
    var fw: Double = 0
    var fh: Double = 0
    var gx: Double = 0
    var gy: Double = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // allocate array
        circleViews = [CircleView]()
        
        // get frame size
        fw = Double(view.frame.width)
        fh = Double(view.frame.height)
        
        // calcuate gaps
        gx = (fw-(padding+padding))/Double(lightsPerRow-1)
        gy = (fh-(paddingTop+padding))/Double(lightsPerColumn-1)
        
        // create all circles
        for y in 0..<lightsPerColumn {
            for x in 0..<lightsPerRow {
                // calculate index
                let idx = y * lightsPerRow + x
                
                // calculate position
                let xx = padding+Double(x)*gx
                let yy = paddingTop+Double(y)*gy
                
                // create view
                let cv = CircleView(frame: CGRect(x: xx-50, y: yy-20, width: 100, height: 40))
                cv.prepare(id: idx + 1)
                
                // add to view
                view.addSubview(cv)
                
                // add to array
                circleViews!.append(cv)
            }
        }
        
        // get settings
        let host = UserDefaults.standard.string(forKey: "host") ?? ""
        let username = UserDefaults.standard.string(forKey: "username")
        let password = UserDefaults.standard.string(forKey: "password")
        
        // create client
        client = CocoaMQTT(clientID: "controller", host: host, port: 1883)
        client!.username = username
        client!.password = password
        client!.delegate = self
        client!.autoReconnect = true
        client!.autoReconnectTimeInterval = 1
        
        // connect to broker
        client!.connect()
    }
    
    @IBAction func stopAll() {
        sendAll(topic: "stop", payload: "")
    }
    
    @IBAction func automateAll() {
        sendAll(topic: "naos/set/automate", payload: "on")
    }
    
    @IBAction func flashAll() {
        sendAll(topic: "flash", payload: "500")
    }
    
    @IBAction func discoAll() {
        sendAll(topic: "disco", payload: "")
    }
    
    func sendAll(topic: String, payload: String) {
        // return immediately if not connected
        if !connected {
            return
        }
        
        // send message to all lights
        for id in 1...48 {
            client!.publish("lights/" + String(id) + "/" + topic, withString: payload)
        }
    }
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        // return immediately if connection has been rejected
        if ack != .accept {
            return
        }
        
        // set flag
        connected = true
        
        // subscribe to topics
        client!.subscribe("lights/+/position")
        client!.subscribe("lights/+/distance")
        client!.subscribe("lights/+/motion")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {
        // get topic segments
        let segments = message.topic.split(separator: "/")
        
        // check for a general topic match
        if segments.count == 3 && segments.first == "lights" {
            // get and check id
            let id = Int(segments[1]) ?? 0
            if id < 1 || id > 48 {
                return
            }
            
            // get circle
            let cv = circleViews![id - 1]
            
            // update circle views based on the received information
            if segments.last == "position" {
                cv.position = Double(String(data: Data(bytes: message.payload), encoding: .utf8) ?? "0") ?? 0
            } else if segments.last == "distance" {
                cv.distance = Double(String(data: Data(bytes: message.payload), encoding: .utf8) ?? "0") ?? 0
            } else if segments.last == "motion" {
                cv.motion = String(data: Data(bytes: message.payload), encoding: .utf8) == "1"
            }
        }
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didSubscribeTopic topic: String) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didUnsubscribeTopic topic: String) {}
    
    func mqttDidPing(_ mqtt: CocoaMQTT) {}
    
    func mqttDidReceivePong(_ mqtt: CocoaMQTT) {}
    
    func mqttDidDisconnect(_ mqtt: CocoaMQTT, withError err: Error?) {
        // set flag
        connected = false
    }
}
