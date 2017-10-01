//
//  ViewController.swift
//  Interface
//
//  Created by Joël Gähwiler on 01.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit

let lightsPerRow = 8
let lightsPerColumn = 6
let lightDotSize = 12
let padding: Double = 150

class ViewController: UIViewController {
    var circles: [[UIView]]?
    var states: [[Bool]]?
    
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
                v.backgroundColor = UIColor(white: 0.25, alpha: 1)
                v.layer.cornerRadius = CGFloat(lightDotSize) / 2.0
                
                // add to view
                view.addSubview(v)
                
                // add to arrays
                circles![y].insert(v, at: x)
                states![y].insert(false, at: x)
            }
        }
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
        
        // TODO: send message
        
        // set state
        states![yy][xx] = true
        
        // animate circle
        UIView.animate(withDuration: 0.25, delay: 0.0, animations: {
            // increase intensity
            v.backgroundColor = UIColor(white: 1, alpha: 1)
        }, completion: { finished in
            UIView.animate(withDuration: 0.25, delay: 0.0, animations: {
                // decrease intensity
                v.backgroundColor = UIColor(white: 0.25, alpha: 1)
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
}
