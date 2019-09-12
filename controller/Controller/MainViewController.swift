//
//  MainViewController.swift
//  Controller
//
//  Created by Joël Gähwiler on 08.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit
import CocoaMQTT

let grid = [
    [ 1,  0,  9,  0, 17,  0],
    [ 0,  5,  0, 13,  0, 21],
    [ 2,  0, 10,  0, 18,  0],
    [ 0,  6, 0,  14,  0, 22],
    [ 3,  0, 11,  0, 19,  0],
    [ 0,  7,  0, 15,  0, 23],
    [ 4,  0, 12,  0, 20,  0],
    [ 0,  8,  0, 16,  0, 24],
]

let rows = grid.count
let columns = grid[0].count

let margin: Double = 150
let marginTop: Double = 200

class MainViewController: UIViewController, CircleViewDelegate, CocoaMQTTDelegate {
    var circleViews: [CircleView]?
    var client: CocoaMQTT?
    var detailVC: DetailViewController?
    
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
        gx = (fw-margin-margin)/Double(columns-1)
        gy = (fh-marginTop-margin)/Double(rows-1)
        
        // create zero circle view
        let zcv = CircleView(frame: CGRect(x: fw-200, y: 100, width: 100, height: 40))
        zcv.delegate = self
        zcv.prepare()
        zcv.id = 0
        
        // add to view
        view.addSubview(zcv)
        
        // add to array
        circleViews!.append(zcv)
        
        // create all circles
        for y in 0..<rows {
            for x in 0..<columns {
                // calculate position
                let xx = margin+Double(x)*gx
                let yy = marginTop+Double(y)*gy
                
                // continue if not in grid
                if grid[y][x] == 0 {
                    continue
                }
                
                // create view
                let cv = CircleView(frame: CGRect(x: xx-50, y: yy-20, width: 100, height: 40))
                cv.delegate = self
                cv.prepare()
                cv.id = grid[y][x]
                
                // add to view
                view.addSubview(cv)
                
                // add to array
                circleViews!.append(cv)
            }
        }
        
        // get settings
        let host = UserDefaults.standard.string(forKey: "host") ?? ""
        let port = UserDefaults.standard.integer(forKey: "port")
        let username = UserDefaults.standard.string(forKey: "username")
        let password = UserDefaults.standard.string(forKey: "password")
        
        // create client
        client = CocoaMQTT(clientID: "controller", host: host, port: UInt16(port))
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
    
    @IBAction func moveAllDown() {
        sendAll(topic: "move", payload: "80")
    }
    
    @IBAction func moveAllUp() {
        sendAll(topic: "move", payload: "140")
    }
    
    @IBAction func automateAll() {
        sendAll(topic: "naos/set/automate", payload: "1")
    }
    
    @IBAction func flashAll() {
        sendAll(topic: "flash", payload: "512 512 512 512 500")
    }
    
    @IBAction func discoAll() {
        let red = String(Int(arc4random_uniform(1023)))
        let green = String(Int(arc4random_uniform(1023)))
        let blue = String(Int(arc4random_uniform(1023)))
        
        sendAll(topic: "fade", payload: String(format: "%@ %@ %@ 0 500", red, green, blue))
    }
    
    // Helpers
    
    func send(id: Int, topic: String, payload: String) {
        // return immediately if not connected
        if !connected {
            return
        }
        
        // send message
        client!.publish("lights/" + String(id) + "/" + topic, withString: payload)
    }
    
    func sendAll(topic: String, payload: String) {
        // return immediately if not connected
        if !connected {
            return
        }
        
        // send message to all lights
        for id in 1...circleViews!.count {
            send(id: id, topic: topic, payload: payload)
        }
    }
    
    func openDetail(id: Int) {
        // kill old view controller
        if detailVC != nil {
            detailVC = nil
        }
        
        // instantiate new view controller
        let storyBoard: UIStoryboard = UIStoryboard(name: "Main", bundle: nil)
        detailVC = (storyBoard.instantiateViewController(withIdentifier: "DetailViewController") as! DetailViewController)
        detailVC!.mainVC = self
        detailVC!.id = id
        
        // copy values from circle view if available
        let cv = circleViews![id]
        detailVC!.position = cv.position
        detailVC!.distance = cv.distance
        detailVC!.motion = cv.motion
        
        // present new view controller
        present(detailVC!, animated: true, completion: nil)
    }
    
    // UIViewController
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
    
    // CircleViewDelegate
    
    func didTapCircleView(sender: CircleView) {
        openDetail(id: sender.id)
    }
    
    // CocoaMQTTDelegate
    
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
        client!.subscribe("lights/+/state")
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
            if id < 0 || id > circleViews!.count {
                return
            }
            
            // get circle view
            var circleView: CircleView?
            for cv in circleViews! {
                if cv.id == id {
                    circleView = cv
                    break
                }
            }
    
            // get detail view controller
            var detailViewController: DetailViewController?
            if detailVC?.id == id {
                detailViewController = detailVC
            }
            
            // update circle view and detail view controller based on the received information
            if segments.last == "position" {
                let position = Double(String(data: Data(bytes: message.payload), encoding: .utf8) ?? "0") ?? 0
                circleView?.position = position
                detailViewController?.position = position
            } else if segments.last == "distance" {
                let distance = Double(String(data: Data(bytes: message.payload), encoding: .utf8) ?? "0") ?? 0
                circleView?.distance = distance
                detailViewController?.distance = distance
            } else if segments.last == "motion" {
                let motion = String(data: Data(bytes: message.payload), encoding: .utf8) == "1"
                circleView?.motion = motion
                detailViewController?.motion = motion
            } else if segments.last == "state" {
                let state = String(data: Data(bytes: message.payload), encoding: .utf8)
                detailViewController?.state = state ?? ""
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
