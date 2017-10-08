//
//  MainViewController.swift
//  Controller
//
//  Created by Joël Gähwiler on 08.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit

let lightsPerRow = 8
let lightsPerColumn = 6
let padding: Double = 150

let offColor = UIColor(white: 0.1, alpha: 1)
let onColor = UIColor(white: 1, alpha: 1)

class MainViewController: UIViewController {
    var container: UIView?
    var circleViews: [[CircleView]]?
    
    var fw: Double = 0
    var fh: Double = 0
    var gx: Double = 0
    var gy: Double = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // create container
        container = UIView(frame: view.frame)
        view.addSubview(container!)
        
        // allocate array
        circleViews = [[CircleView]]()
        
        // get frame size
        fw = Double(container!.frame.width)
        fh = Double(container!.frame.height)
        
        // calcuate gaps
        gx = (fw-(2.0*padding))/Double(lightsPerRow-1)
        gy = (fh-(2.0*padding))/Double(lightsPerColumn-1)
        
        // create all circles
        for y in 0..<lightsPerColumn {
            // add rows
            circleViews!.insert([CircleView](), at: y)
            
            for x in 0..<lightsPerRow {
                // calculate position
                let xx = padding+Double(x)*gx
                let yy = padding+Double(y)*gy
                
                // create view
                let cv = CircleView(frame: CGRect(x: xx-50, y: yy-20, width: 100, height: 40))
                cv.prepare(id: y * lightsPerRow + x + 1)
                
                // add to view
                container!.addSubview(cv)
                
                // add to arrays
                circleViews![y].insert(cv, at: x)
            }
        }
    }
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
}
