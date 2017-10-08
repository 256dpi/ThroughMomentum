//
//  Circle.swift
//  Controller
//
//  Created by Joël Gähwiler on 08.10.17.
//  Copyright © 2017 Through Momentum. All rights reserved.
//

import UIKit

protocol CircleViewDelegate {
    func didTapCircleView(sender: CircleView)
}

class CircleView: UIView {
    var delegate: CircleViewDelegate?
    var circleView: UIView?
    var idLabel: UILabel?
    var positionLabel: UILabel?
    var distanceLabel: UILabel?
    
    var id: Int {
        get { return Int(idLabel?.text ?? "0") ?? 0 }
        set { idLabel?.text = String(format: "%02d", newValue) }
    }
    
    var position: Double {
        get { return Double(positionLabel?.text ?? "0") ?? 0 }
        set { positionLabel?.text = String(format: "%.1f", newValue) }
    }
    
    var distance: Double {
        get { return Double(distanceLabel?.text ?? "0") ?? 0 }
        set { distanceLabel?.text = String(format: "%.1f", newValue) }
    }
    
    var motion: Bool {
        get { return circleView?.layer.borderWidth == 3 }
        set { circleView!.layer.borderWidth = newValue ? 3 : 1 }
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("This class does not support NSCoding")
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
    }
    
    func prepare() {
        // set background color
        backgroundColor = UIColor.clear
    
        // add arc view
        circleView = UIView(frame: CGRect(x: 0, y: 0, width: frame.height, height: frame.height))
        circleView!.layer.cornerRadius = frame.height/2
        circleView!.layer.borderWidth = 1.0
        circleView!.layer.borderColor = UIColor.white.cgColor
        addSubview(circleView!)
        
        // add text view
        idLabel = UILabel(frame: CGRect(x: 0, y: 0, width: frame.height, height: frame.height))
        idLabel!.textAlignment = .center
        idLabel!.font = idLabel!.font.withSize(14)
        idLabel!.textColor = .white
        idLabel!.text = "00"
        addSubview(idLabel!)
        
        // add position label
        positionLabel = UILabel(frame: CGRect(x: frame.width / 2 + 5, y: frame.height / 5, width: frame.width / 2 - 5, height: frame.height / 5))
        positionLabel!.textAlignment = .left
        positionLabel!.font = positionLabel!.font.withSize(10)
        positionLabel!.textColor = .white
        positionLabel!.text = "0.0"
        addSubview(positionLabel!)
        
        // add distance label
        distanceLabel = UILabel(frame: CGRect(x: frame.width / 2 + 5, y: frame.height / 5 * 3, width: frame.width / 2 - 5, height: frame.height / 5))
        distanceLabel!.textAlignment = .left
        distanceLabel!.font = distanceLabel!.font.withSize(10)
        distanceLabel!.textColor = .white
        distanceLabel!.text = "0.0"
        addSubview(distanceLabel!)
        
        // add gesture recognizer
        addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.didTap(_:))))
    }
    
   @objc func didTap(_ sender: UITapGestureRecognizer) {
        // call delegate if available
        if let d = delegate {
            d.didTapCircleView(sender: self)
        }
    }
}
