//
//  ViewController.swift
//  Interface
//
//  Created by Joël Gähwiler on 01.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit
import CocoaMQTT

let lightsPerRow = 8
let lightsPerColumn = 6
let lightDotSize = 12
let padding: Double = 150

let offColor = UIColor(white: 0.1, alpha: 1)
let onColor = UIColor(white: 1, alpha: 1)

class ViewController: UIViewController, CocoaMQTTDelegate {
    var circles: [[UIView]]?
    var states: [[Bool]]?
    var client: CocoaMQTT?
    var connected = false
    
    var fw: Double = 0
    var fh: Double = 0
    var gx: Double = 0
    var gy: Double = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // allocate arrays
        circles = [[UIView]]()
        states = [[Bool]]()
        
        // get frame size
        fw = Double(view.frame.width)
        fh = Double(view.frame.height)
        
        // calcuate gaps
        gx = (fw-(2.0*padding))/Double(lightsPerRow-1)
        gy = (fh-(2.0*padding))/Double(lightsPerColumn-1)
        
        // create all circles
        for y in 0..<lightsPerColumn {
            // add rows
            circles!.insert([UIView](), at: y)
            states!.insert([Bool](), at: y)
            
            for x in 0..<lightsPerRow {
                // calculate position
                let xx = padding+Double(x)*gx
                let yy = padding+Double(y)*gy
                
                // create view
                let v = UIView(frame: CGRect(x: xx, y: yy, width: Double(lightDotSize), height: Double(lightDotSize)))
                v.backgroundColor = offColor
                v.layer.cornerRadius = CGFloat(lightDotSize) / 2.0
                
                // add to view
                view.addSubview(v)
                
                // add to arrays
                circles![y].insert(v, at: x)
                states![y].insert(false, at: x)
            }
        }
        
        // create client
        client = CocoaMQTT(clientID: "interface", host: "broker.shiftr.io", port: 1883)
        client!.username = "96c342e4"
        client!.password = "1724bcdee75a6f0b"
        client!.delegate = self
        
        // connect to broker
        client!.connect()
    }
    
    func handleTouches(touches: Set<UITouch>) {
        // handle all touches
        touches.forEach { (touch) in
            handleTouch(touch: touch)
        }
    }
    
    func handleTouch(touch: UITouch) {
        // get location
        let x = Double(touch.location(in: view).x) - Double(padding)
        let y = Double(touch.location(in: view).y) - Double(padding)
        
        // calculate row and column number
        let xx = Int(round(x / gx))
        let yy = Int(round(y / gy))
        
        // check bounds
        if xx < 0 || xx >= lightsPerRow || yy < 0 || yy >= lightsPerColumn {
            return
        }
        
        // check state
        if states![yy][xx] {
            return
        }
        
        // get view from array
        let v = circles![yy][xx]
        
        // send message
        if connected {
            client!.publish("activate", withString: String(yy*lightsPerRow+xx))
        }
        
        // set state
        states![yy][xx] = true
        
        // animate circle
        UIView.animate(withDuration: 0.25, delay: 0.0, animations: {
            // increase intensity
            v.backgroundColor = onColor
        }, completion: { finished in
            UIView.animate(withDuration: 0.25, delay: 0.0, animations: {
                // decrease intensity
                v.backgroundColor = offColor
            }, completion: { finished in
                // reset state
                self.states![yy][xx] = false
            })
        })
    }
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouches(touches: touches)
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouches(touches: touches)
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouches(touches: touches)
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didConnect host: String, port: Int) {
        connected = true
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        // set flag
        connected = true
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didSubscribeTopic topic: String) {}
    
    func mqtt(_ mqtt: CocoaMQTT, didUnsubscribeTopic topic: String) {}
    
    func mqttDidPing(_ mqtt: CocoaMQTT) {}
    
    func mqttDidReceivePong(_ mqtt: CocoaMQTT) {}
    
    func mqttDidDisconnect(_ mqtt: CocoaMQTT, withError err: Error?) {
        // set flag
        connected = false
        
        // attempt reconnect in one second
        DispatchQueue.main.asyncAfter(deadline: DispatchTime.now()+1) {
            self.client!.connect()
        }
    }
}
